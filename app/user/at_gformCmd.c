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
#include "../include/driver/uart.h"
#include "user_interface.h"
#include "espconn.h"

char gFormID[64];
uint8_t fieldSize;
char gFormEntryIDs[8][24];
static struct espconn *conn;

void ICACHE_FLASH_ATTR
appendChar(char* s, size_t size, char c) {
	s[size]=c;
	s[size+1]='\0';
}

uint8_t ICACHE_FLASH_ATTR
appendStr(char* s, uint8_t size, char* c) {
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
	char fieldSizeChar[3];
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

	if(fieldSize > 8) {
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
at_exeCmdGformsubmit(uint8_t id) {
	uint8_t tmpSize, i;
	char tmp[256];

	tmpSize=os_sprintf(tmp, "https://docs.google.com/forms/d/%s/formResponse?ifq", gFormID);
	tmp[tmpSize]='\0';
	for(i=0; i<=fieldSize-1; i++) {
		appendChar(tmp, tmpSize, '&'); tmpSize++;
		uint8_t j;
		for(j=0; j<=os_strlen(gFormEntryIDs[i])-1; j++) {
			appendChar(tmp, tmpSize, gFormEntryIDs[i][j]); tmpSize++;
		}
		appendChar(tmp, tmpSize, '='); tmpSize++;
		// TODO: Add corresponding data
	}
	tmpSize=appendStr(tmp, tmpSize, "&submit=Submit");

	uart0_sendStr(tmp);

	at_backOk;
}
