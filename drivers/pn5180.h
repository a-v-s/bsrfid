/******************************************************************************\

File:         pn5180.h
Author:       André van Schoubroeck
License:      MIT

This implements the PN5180 family of RFID reader ICs

* PN5180
* Other compatibles 
********************************************************************************
MIT License

Copyright (c) 2020 André van Schoubroeck <andre@blaatschaap.be>

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


Placeholder for planned future implementation.

*******************************************************************************/


#include <stdint.h>

#include "pdc.h"
typedef bs_pdc_t pn5180_t ;

#define PN5180_CMD_WRITE_REGISTER				(0x00)
#define PN5180_CMD_WRITE_REGISTER_OR_MASK		(0x01)
#define PN5180_CMD_WRITE_REGISTER_AND_MASK		(0x02)
#define PN5180_CMD_WRITE_REGISTER_MULTIPLE		(0x03)
#define PN5180_CMD_READ_REGISTER				(0x04)
#define PN5180_CMD_READ_REGISTER_MULTIPLE		(0x05)
#define PN5180_CMD_WRITE_EEPROM					(0x06)
#define PN5180_CMD_READ_EEPROM					(0x07)
#define PN5180_CMD_WRITE_TX_DATA				(0x08)
#define PN5180_CMD_SEND_DATA					(0x09)
#define PN5180_CMD_READ_DATA					(0x0A)
#define PN5180_CMD_SWITCH_MODE					(0x0B)
#define PN5180_CMD_MIFARE_AUTHENTICATE			(0x0C)
#define PN5180_CMD_EPC_INVENTORY				(0x0D)
#define PN5180_CMD_EPC_RESUME_INVENTORY			(0x0E)
#define PN5180_CMD_EPC_RETRIEVE_INVENTORY_RESULT_SIZE	(0x0F)

#define PN5180_CMD_EPC_RETRIEVE_INVENTORY_RESULT	(0x10)
#define PN5180_CMD_LOAD_RF_CONFIG					(0x11)
#define PN5180_CMD_UPDATE_RF_CONFIG					(0x12)
#define PN5180_CMD_RETRIEVE_RF_CONFIG_SIZE			(0x13)
#define PN5180_CMD_RETRIEVE_RF_CONFIG				(0x14)
#define PN5180_CMD_RFU15							(0x15)
#define PN5180_CMD_RF_ON							(0x16)
#define PN5180_CMD_RF_OFF							(0x17)
#define PN5180_CMD_CONFIGURE_TESTBUS_DIGITAL		(0x18)
#define PN5180_CMD_CONFIGURE_TESTBUS_ANALOG			(0x19)

#pragma pack(push,1)

typedef struct {
	uint8_t command;
	union {
		struct {
			uint8_t start_address;
			uint8_t number_of_bytes;
		} read_eeprom;
		struct {
			uint8_t address;
			uint32_t value;
		} reg;
		uint8_t raw[260];
	};
} pn5180_request_t;

typedef struct {
	uint8_t die_identifier[0x10];
	uint16_t product_version;
	uint16_t firmware_version;
	uint16_t eeprom_version;
} pn5180_eeprom_info;



int pn5180_get_reg32(pn5180_t *pn5180, uint8_t reg, uint32_t *value) ;
int pn5180_set_reg32(pn5180_t *pn5180, uint8_t reg, uint32_t value) ;

int pn5180_or_reg32(pn5180_t *pn5180, uint8_t reg, uint32_t value) ;
int pn5180_and_reg32(pn5180_t *pn5180, uint8_t reg, uint32_t value) ;
#pragma pack(pop)
