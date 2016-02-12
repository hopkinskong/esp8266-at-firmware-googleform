An ESP8266 SDK Firmware with AT command to submit data to Google Form.

### Features ###
- Directly submitting data to Google Form.
- SSL implemented for submitting data (As Google forced it)

### USAGE ###
`AT+GFORMSUBMIT=<Google_form_ID>,<Total_entries_n>,<Entry_1_data>,<Entry_2_data>,<Entry_n_data>,...`

Where:

`<Google_form_ID>` = Your Google Form ID

`<Total_entries_n>` = Total entries that will be submitted to Google Form, usually this equals to how many text box/input in your form

`<Entry_n_data>` = Data to be submitted, must in the form of name and value pair, i.e.: entry.1335153026=data_for_this_entry

### EXAMPLE ###
- Given the form link is:

	[https://docs.google.com/forms/d/1bg-8CAyVF3Rq3Q_1Z87BLxUrE4aKB1AEMhkXyQAl-u4/viewform](https://docs.google.com/forms/d/1bg-8CAyVF3Rq3Q_1Z87BLxUrE4aKB1AEMhkXyQAl-u4/viewform)

- `<Google_form_ID>` = `1bg-8CAyVF3Rq3Q_1Z87BLxUrE4aKB1AEMhkXyQAl-u4`

- `<Total_entries_n>` = `2`

- `Entry IDs` = `entry.1335153026`, `entry.1490820559`

	(Try to view source code of the form, or follow [this](https://support.google.com/docs/answer/160000?hl=en) to get the pre-filled link to extract the entry IDs)

Execute the following command to submit the Google Form

`AT+GFORMSUBMIT=1bg-8CAyVF3Rq3Q_1Z87BLxUrE4aKB1AEMhkXyQAl-u4,2,entry.1335153026=answer_for_question_one,entry.1490820559=answer_for_question_two`

### NOTE ###
- Try not to put big (lengthy) data, the ESP8266 does not have much RAM after we used the SSL library
- Request packet buffer size is `224`, could be changed in `at_gformCmd.c`, but it is recommended not to modify the value.
- SSL buffer size is `4608`, could be changed in `at_gformCmd.c`, but it is recommended not to modify the value.
- Comma (,) is not allowed in any fields including `<Google_form_ID>`, 
`<Entry_n_data>`
- Remember to put \r\n after each command (complies with the original AT FW specifications)

### KNOWN BUG ###
- Application may reset when sending command too fast
- Need to add packet echo to avoid data drop (very weird)