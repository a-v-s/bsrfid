/******************************************************************************\

File:         st25r95.h
Author:       André van Schoubroeck
License:      MIT

This implements the STM ST25R95 family of RFID reader ICs

* STM CR95HF
* STM ST95HF
* STM ST25R95
* Other compatibles 
********************************************************************************
MIT License

Copyright (c) 2020, 2022 André van Schoubroeck <andre@blaatschaap.be>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
********************************************************************************

This file is a placeholder for planned support.


https://community.st.com/s/question/0D53W00000Bsjx4SAB/differences-between-st95hf-and-st25r95

Differences:
+----------+-----+------+----------------+
| Device   | SPI | Uart | Card Emulation |
+----------+-----+------+----------------+ 
| CR95HF   | Yes | Yes  | No             | 
| ST95HF   | Yes | No   | Yes            | 
| ST25R95  | Yes | No   | Yes            | 
+----------+-----+------+----------------+

*******************************************************************************/

#define ST25R95_CMD_IDN				(0x01)	//
#define ST25R95_CMD_PROTOCOLSELECT	(0x02)
#define ST25R95_CMD_POLLFIELD 		(0x03)
#define ST25R95_CMD_SendRecv 		(0x04)
#define ST25R95_CMD_LISTEN 			(0x05)
#define ST25R95_CMD_SEND 			(0x06)
#define ST25R95_CMD_IDLE 			(0x07)
#define ST25R95_CMD_RDREG 			(0x08)
#define ST25R95_CMD_WRREG 			(0x09)
#define ST25R95_CMD_BaudRate 		(0x0A)
#define ST25R95_CMD_SubFreqRes 		(0x0B)
#define ST25R95_CMD_ACFILTER 		(0x0D)
#define ST25R95_CMD_Echo 			(0x55)

#define ST25R95_VAL_PROTOCOLSELECT_FIELD_OFF	(0x00)
#define ST25R95_VAL_PROTOCOLSELECT_ISO_15693	(0x01)
#define ST25R95_VAL_PROTOCOLSELECT_ISO_14443A	(0x02)
#define ST25R95_VAL_PROTOCOLSELECT_ISO_14443B	(0x03)
#define ST25R95_VAL_PROTOCOLSELECT_ISO_18092	(0x04)

typedef enum {
	speed_26_kbps = 0b00,
	speed_52_kbps = 0b01,
	speed_6_kbps = 0b10,
} st25r95_val_protocolselect_parameters_iso15693_speed_t;

typedef union {
	uint8_t field_off;
	struct {
		unsigned int append_crc : 1;
		unsigned int single_or_dual_subcarrier :1;
		unsigned int modulation_10_or_100 : 1;
		unsigned int delay_or_wait : 1;
		st25r95_val_protocolselect_parameters_iso15693_speed_t speed : 2;
	} iso15693;
	struct {} iso14443a;
	struct {} iso14443b;
	struct {} iso18092;
} st25r95_val_protocolselect_parameters_t;


