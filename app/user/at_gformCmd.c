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

/*
 *  Tested value:
 *  3072 FAIL, IMMEDIATE DISCONNECTED, NEED TO INCREASE
 *  3584 FAIL, IMMEDIATE DISCONNECTED, NEED TO INCREASE
 *  4096 OK, BUT SOME DATA DROPPED, MAYBE CASUED BY INSUFFICIENT MEMORY/SSL_BUFFER_SIZE TOO SMALL??
 */

#define SSL_BUFFER_SIZE 5120 // MAX=8192, but beyond 4096 may cause insufficient memory for packet assembly, so i recommend not to increase it.
#define PACKET_BUFFER_SIZE 224

#define GOOGLE_FORM_DOMAIN "docs.google.com"
#define GOOGLE_FORM_DOMAIN_PORT 443 // 443 for https

os_timer_t timer;

char *gFormData;
struct espconn conn;
struct _esp_tcp tcp;
static ip_addr_t ip_addr;

unsigned char *default_certificate;
unsigned int default_certificate_len = 0;
unsigned char *default_private_key;
unsigned int default_private_key_len = 0;

extern uint8_t at_wifiMode;

uint16_t ICACHE_FLASH_ATTR
appendChar(char* s, uint16_t size, char c) {
	s[size]=c;
	s[size+1]='\0';
	size++;
	return size;
}

uint16_t ICACHE_FLASH_ATTR
appendStr(char* s, uint16_t size, char* c) {
	uint8_t i=0;
	while(c[i] != '\0') {
		size=appendChar(s, size, c[i]);
		i++;
	}
	return size;
}

void ICACHE_FLASH_ATTR
disconnect_callback(void *arg) {
	// Disconnection succeed
	  uart0_sendStr("DISCONNECTED\r\n");
}

void ICACHE_FLASH_ATTR
data_receive_callback(void *arg, char *data, unsigned short length) {
	// Received data
	//uart0_tx_buffer(data, length);
	espconn_secure_disconnect((struct espconn *)arg);
}

void ICACHE_FLASH_ATTR
data_sent_callback(void *arg) {
	// Sent data
	uart0_sendStr("\r\nSEND OK\r\n");
}

void ssl_send_data(struct espconn *_conn) {
	char *fieldSizeChar;
	uint8_t i, index, fieldSize;
	uint16_t requestPathSize;
	char *tmp;

	char *packetBuffer = (char *)os_zalloc(PACKET_BUFFER_SIZE);
	uint16_t packetBufferSize=0;

	/*
	 * Creating request header
	 * Format: GET /forms/d/<google_form_id>/formResponse?ifq&<ent_id>=<ent_data>&<ent_id>=<ent_data>...&submit=Submit
	 */

	// Appending packet data
	packetBufferSize=appendStr(packetBuffer, packetBufferSize, "GET /forms/d/");

	// Appending Google Form ID
	tmp = os_strchr(gFormData, ',');
	index = (uint8_t)(tmp - gFormData);
	for(i=0; i<=index-1; i++) {
		packetBufferSize=appendChar(packetBuffer, packetBufferSize, gFormData[0]);
		gFormData++;
	}
	gFormData++; // Get pass ","

	// Appending packet data
	packetBufferSize=appendStr(packetBuffer, packetBufferSize, "/formResponse?ifq");

	// Converting total entries to uint8_t
	fieldSizeChar = (char *)os_zalloc(2); // This limits the maximum entries are 9. It is already pretty enough.
	tmp = os_strchr(gFormData, ',');
	index = (uint8_t)(tmp - gFormData);
	for(i=0; i<=index-1; i++) {
		fieldSizeChar[i]=gFormData[0];
		gFormData++;
	}
	fieldSizeChar[++i]='\0';
	fieldSize = atoi(fieldSizeChar);
	gFormData++; // Get pass ","
	os_free(fieldSizeChar); // Free precious memories

	// Make sure entry size is in-range
	if(fieldSize > 9 || fieldSize <= 0) {
		at_backError;
		return;
	}

	// Appending form data
	for(i=0; i<=fieldSize-1; i++) {
		uint8_t j;
		packetBufferSize=appendChar(packetBuffer, packetBufferSize, '&');
		// Add corresponding data
		tmp=os_strchr(gFormData, ',');
		index = (uint8_t)(tmp-gFormData);
		for(j=0; j<=index-1; j++) {
			if(gFormData[0] == '\r' || gFormData[0] == '\n') { // Skips \r or \n
				continue;
			}
			packetBufferSize=appendChar(packetBuffer, packetBufferSize, gFormData[0]);
			gFormData++;
		}
		gFormData++; // Get pass ","
	}

	// Appending packet data
	packetBufferSize=appendStr(packetBuffer, packetBufferSize, "&submit=Submit HTTP/1.1");
	packetBufferSize=appendChar(packetBuffer, packetBufferSize, '\r');
	packetBufferSize=appendChar(packetBuffer, packetBufferSize, '\n');
	packetBufferSize=appendStr(packetBuffer, packetBufferSize, "Host: ");
	packetBufferSize=appendStr(packetBuffer, packetBufferSize, GOOGLE_FORM_DOMAIN);
	packetBufferSize=appendChar(packetBuffer, packetBufferSize, '\r');
	packetBufferSize=appendChar(packetBuffer, packetBufferSize, '\n');
	packetBufferSize=appendChar(packetBuffer, packetBufferSize, '\r');
	packetBufferSize=appendChar(packetBuffer, packetBufferSize, '\n');

	uart0_sendStr("\r\nRequest packet:"); uart0_sendStr(packetBuffer); uart0_sendStr("\r\n");
	espconn_secure_sent(_conn, packetBuffer, packetBufferSize); // Send the packet out
	os_free(packetBuffer); // Free the packet buffer
	os_free(tmp);
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
