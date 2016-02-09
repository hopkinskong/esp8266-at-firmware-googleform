/*
 * at_gformCmd.h
 *
 *  Created on: 2016¦~2¤ë8¤é
 *      Author: hopkins
 */

#ifndef APP_USER_AT_GFORMCMD_H_
#define APP_USER_AT_GFORMCMD_H_

// AT Commands for Google Form
/*
 * USAGE:
 * AT+GFORMSET=<Google Form ID>,<Entry Count(MAX n)>,<Entry_1>,<Entry_2>,<Entry_n>,...
 * After running AT+GFORMSET, execute AT+GFORMSUBMIT:
 * AT+GFORMSUBMIT=<Entry_n_data>,<Entry_1_data>,<Entry_2_data>,<Entry_n_data>,...
 *
 * NOTE:
 * Maximum entry count is 8, each entry should contain no more than 23 chars
 * Maximum Google Form ID chars is 63
 * Don't put too big data (lengthy data) in GFORMSUBMIT
 * Comma (,) is not allowed in any fields including form ID, entry names and entry data
 * Remember to put \r\n after each command (complies with the original AT FW specifications)
 *
 * EXAMPLE:
 * AT+GFORMSET=1bg-8CAyVF3Rq3Q_1Z87BLxUrE4aKB1AEMhkXyQAl-u4,2,entry.1335153026,entry.1490820559
 * AT+GFORMSUBMIT=answer_1,answer_2
 */

void at_setupCmdGformset(uint8_t id, char *pPara);
void at_setupCmdGformsubmit(uint8_t id, char *pPara);

#endif /* APP_USER_AT_GFORMCMD_H_ */
