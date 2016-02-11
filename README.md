An ESP8266 SDK Firmware with AT command to submit data to Google Form.

Currently, the firmware is still not fully completed, only partially functionable.
USE AT YOUR OWN RISK.
There are problems in my program structure which will end up using lots of memories.
The program may work on the first time, but it may then fail.
Some data may be even only partially transferred.
Some debug info/response data will be displayed via UART, these will be removed later.

Features:
- Directly submitting data to Google Form.
- SSL implemented for submitting data (As Google forced it)

 * USAGE:
 * AT+GFORMSET=<Google Form ID>,<Entry Count(MAX n)>,<Entry_1>,<Entry_2>,<Entry_n>,...
 * After running AT+GFORMSET, execute AT+GFORMSUBMIT:
 * AT+GFORMSUBMIT=<Entry_n_data>,<Entry_1_data>,<Entry_2_data>,<Entry_n_data>,...
 *
 * NOTE:
 * Maximum entry count is 5, each entry should contain no more than 19 chars
 * (For saving RAM, or you will meet unexpected behaviour)
 * Maximum Google Form ID chars is 59
 * Don't put too big data (lengthy data) in GFORMSUBMIT
 * Comma (,) is not allowed in any fields including form ID, entry names and entry data
 * Remember to put \r\n after each command (complies with the original AT FW specifications)
 *
 * EXAMPLE:
 * AT+GFORMSET=1bg-8CAyVF3Rq3Q_1Z87BLxUrE4aKB1AEMhkXyQAl-u4,2,entry.1335153026,entry.1490820559
 * AT+GFORMSUBMIT=answer_1,answer_2