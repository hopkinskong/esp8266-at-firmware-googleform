/*
 * at_gformCmd.c
 *
 *  Created on: 2016¦~2¤ë8¤é
 *      Author: hopkins
 */

#include "osapi.h"
#include "c_types.h"
#include "at.h"
#include "at_gformCmd.h"
#include "driver/uart.h"
#include "user_interface.h"
#include "mem.h"
#include "../include/espconn.h"

#define SSL_BUFFER_SIZE 4096 // MAX=8192, but beyond 4096 may cause insufficient memory for packet assembly, so i recommend not to increase it.

#define GOOGLE_FORM_DOMAIN "docs.google.com"
#define GOOGLE_FORM_DOMAIN_PORT 443 // 443 for https

os_timer_t timer;

char gFormID[60];
uint8_t fieldSize;
char gFormEntryIDs[5][20];
char *gFormData;
struct espconn conn;
struct _esp_tcp tcp;
static ip_addr_t ip_addr;
const char* REQUEST_HEADER = "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n";

unsigned char *default_certificate;
unsigned int default_certificate_len = 0;
unsigned char *default_private_key;
unsigned int default_private_key_len = 0;

extern uint8_t at_wifiMode;

void ICACHE_FLASH_ATTR
appendChar(char* s, uint16_t size, char c) {
	s[size]=c;
	s[size+1]='\0';
}

uint8_t ICACHE_FLASH_ATTR
appendStr(char* s, uint16_t size, char* c) {
	uint8_t i=0;
	while(c[i] != '\0') {
		appendChar(s, size, c[i]);
		size++;
		i++;
	}
	return size;
}

void ICACHE_FLASH_ATTR
at_setupCmdGformset(uint8_t id, char *pPara) {
	char fieldSizeChar[2];
	uint8_t i, j, index;

	pPara++; // Get pass "="

	char *tmp = os_strchr(pPara, ',');
	index = (uint8_t)(tmp - pPara);
	for(i=0; i<=index-1; i++) {
		gFormID[i]=pPara[0];
		pPara++;
	}
	gFormID[++i]='\0';
	pPara++; // Get pass ","

	tmp = os_strchr(pPara, ',');
	index = (uint8_t)(tmp - pPara);
	for(i=0; i<=index-1; i++) {
		fieldSizeChar[i]=pPara[0];
		pPara++;
	}
	fieldSizeChar[++i]='\0';
	fieldSize = atoi(fieldSizeChar);
	pPara++; // Get pass ","

	if(fieldSize > 5) {
		at_backError;
	}else{
		for(j=0; j<=fieldSize-1; j++) {
			tmp = os_strchr(pPara, ',');
			index = (uint8_t)(tmp - pPara);
			for(i=0; i<=index-1; i++) {
				if(pPara[0] == '\r' || pPara[0] == '\n') {
					continue;
				}
				gFormEntryIDs[j][i]=pPara[0];
				pPara++;
			}
			gFormEntryIDs[j][++i]='\0';
			pPara++; // Get pass ","
		}
		at_backOk;
	}
}

void ICACHE_FLASH_ATTR
disconnect_callback(void *arg) {
	// Disconnection succeed
	  uart0_sendStr("DISCONNECTED\r\n");
}

void ICACHE_FLASH_ATTR
data_receive_callback(void *arg, char *data, unsigned short length) {
	// Received data
	uart0_tx_buffer(data, length);
}

void ICACHE_FLASH_ATTR
data_sent_callback(void *arg) {
	// Sent data
	uart0_sendStr("\r\nSEND OK\r\n");
}

void ssl_send_data(struct espconn *_conn) {
	char requestPath[160];
	uint8_t i, index;
	uint16_t requestPathSize;
	char *tmp;

	char *packetBuffer = (char *)os_zalloc(SSL_BUFFER_SIZE);
	requestPathSize=os_sprintf(requestPath, "/forms/d/%s/formResponse?ifq", gFormID);
	requestPath[requestPathSize]='\0';
	for(i=0; i<=fieldSize-1; i++) {
		appendChar(requestPath, requestPathSize, '&'); requestPathSize++;
		uint8_t j;
		for(j=0; j<=os_strlen(gFormEntryIDs[i])-1; j++) {
			appendChar(requestPath, requestPathSize, gFormEntryIDs[i][j]); requestPathSize++;
		}
		appendChar(requestPath, requestPathSize, '='); requestPathSize++;
		// Add corresponding data
		tmp=os_strchr(gFormData, ',');
		index = (uint8_t)(tmp-gFormData);
		for(j=0; j<=index-1; j++) {
			appendChar(requestPath, requestPathSize, gFormData[0]); requestPathSize++;
			gFormData++;
		}
		gFormData++; // Get pass ","
	}
	requestPathSize=appendStr(requestPath, requestPathSize, "&submit=Submit");
	os_sprintf(packetBuffer, REQUEST_HEADER, requestPath, GOOGLE_FORM_DOMAIN); // Assemble the packet by putting the request path and host name
	uart0_sendStr("\r\nRQP:"); uart0_sendStr(packetBuffer); uart0_sendStr("\r\n");
	espconn_secure_sent(_conn, packetBuffer, os_strlen(packetBuffer)); // Send the packet out
	os_free(packetBuffer); // Free the packet buffer
}

void ICACHE_FLASH_ATTR
connect_callback(void *arg) {
	// Connection established
	struct espconn *_conn = (struct espconn *)arg;
	espconn_regist_recvcb(_conn, data_receive_callback);
	espconn_regist_sentcb(_conn, data_sent_callback);
	ssl_send_data(_conn);
}

void ICACHE_FLASH_ATTR
reconnect_callback(void *arg, sint8 err) {
	// Error occurred, TCP connection broke.
	// May try to reconnect or do other error handling.
	char tmp[11];
	uart0_sendStr("CLOSED\r\n");
}

void ICACHE_FLASH_ATTR
dns_found(const char *name, ip_addr_t *ipaddr, void *arg) {
	struct espconn *_conn = (struct espconn *)arg;
	if(ipaddr == NULL) {
	    uart0_sendStr("DNS Fail\r\n");
	    return;
	}
	if(ip_addr.addr == 0 && ipaddr->addr != 0) {
		// Got IP, start TCP connection to Google server
		os_timer_disarm(&timer);
		ip_addr.addr = ipaddr->addr;
		os_memcpy(_conn->proto.tcp->remote_ip, &ipaddr->addr, 4);
		_conn->proto.tcp->remote_port = GOOGLE_FORM_DOMAIN_PORT;
		_conn->proto.tcp->local_port = espconn_port(); // Request a free local port
		espconn_regist_connectcb(_conn, connect_callback); // Registering connect callback
		espconn_regist_reconcb(_conn, reconnect_callback); // Registering reconnect callback (as error handler)
		espconn_regist_disconcb(_conn, disconnect_callback); // Registering disconnect callback (as error handler for SSL handshake failure)
		espconn_secure_set_size(0x01, SSL_BUFFER_SIZE); // Increase SSL buffer size (default=2048, not enough by experiment)
		espconn_secure_connect(_conn);
	}
}

void ICACHE_FLASH_ATTR
dns_check_callback(void *arg) {
	struct espconn *_conn = arg;
	espconn_gethostbyname(_conn, GOOGLE_FORM_DOMAIN, &ip_addr, dns_found);
	os_timer_arm(&timer, 1000, 0);
}

void ICACHE_FLASH_ATTR
at_setupCmdGformsubmit(uint8_t id, char *pPara) {
	os_timer_disarm(&timer); // Disarm timer if exists
	enum espconn_type linkType = ESPCONN_TCP;
	uint32_t ip = 0;
	if(at_wifiMode == 1) {
		if(wifi_station_get_connect_status() != STATION_GOT_IP) {
			uart0_sendStr("no ip\r\n");
			return;
		}
	}else{
		at_backError;
		return;
	}
	pPara++; // Get pass "="
	gFormData=pPara;

	conn.proto.tcp=&tcp;
	conn.type=ESPCONN_TCP;
	conn.state=ESPCONN_NONE;

	// Use DNS to get IP address of docs.google.com
	ip_addr.addr=0;
	espconn_gethostbyname(&conn, GOOGLE_FORM_DOMAIN, &ip_addr, dns_found);
	// Set timer for DNS resolution
	os_timer_setfn(&timer, (os_timer_func_t *)dns_check_callback, conn);
	os_timer_arm(&timer, 1000, 0);
}
