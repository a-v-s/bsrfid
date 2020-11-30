/*
 * rc52x.c
 *
 *  Created on: 29 nov. 2020
 *      Author: andre
 */


// For testing purposes, thsi will not be the final api

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "rc52x_transport.h"
#include "rc52x.h"

#include "iso14443a.h"



int rc52x_get_chip_version(rc52x_t *rc52x, uint8_t* chip_id) {
	return mfrc522_recv(rc52x, MFRC_REG_VersionReg, chip_id, 1);
}

int rc52x_init(rc52x_t *rc52x) {
	// For initial testing, a sequence of commands found
	// in common implementations

	rc52x_set_reg8(rc52x, MFRC_REG_TModeReg, 0x8D);
	rc52x_set_reg8(rc52x, MFRC_REG_TPrescalerReg, 0x3E);
	rc52x_set_reg8(rc52x, MFRC_REG_TReloadReg_Lo, 30);
	rc52x_set_reg8(rc52x, MFRC_REG_TReloadReg_Hi, 0);
	rc52x_set_reg8(rc52x, MFRC_REG_RFCfgReg, 0x70);			// 48dB gain
	rc52x_set_reg8(rc52x, MFRC_REG_TxASKReg, 0x40);
	rc52x_set_reg8(rc52x, MFRC_REG_ModeReg, 0x3D);
	rc52x_or_reg8(rc52x, MFRC_REG_TxControlReg, 0x03); // Antenna On
}

int rc52x_anticol(rc52x_t *rc52x, uint8_t * buffer) {

	rc52x_set_reg8(rc52x,MFRC_REG_BitFramingReg, 0x07);
	buffer[0] =  PICC_CMD_REQA;
	rc52x_set_reg8(rc52x,MFRC_REG_ComlEnReg,  0x80);
	rc52x_and_reg8(rc52x,MFRC_REG_ComIrqReg,~0x80);
	rc52x_or_reg8(rc52x,MFRC_REG_FIFOLevelReg, 0x80);
	rc52x_set_reg8(rc52x,MFRC_REG_CommandReg, MFRC_Idle);
	mfrc522_send(rc52x,MFRC_REG_FIFODataReg,buffer, 1);
	rc52x_set_reg8(rc52x,MFRC_REG_CommandReg, MFRC_Transceive);

	rc52x_or_reg8(rc52x,MFRC_REG_BitFramingReg, 0x80);


	delay_ms(100);
	rc52x_and_reg8(rc52x,MFRC_REG_BitFramingReg, ~0x80);



	rc52x_set_reg8(rc52x,MFRC_REG_BitFramingReg, 0x00);

	// Let's start simple and follow the implementation by
	// https://github.com/song940/RFID-RC522
	// Later we'll move on to a complete anti collision and
	// cascading implementation like is done in
	// https://github.com/miguelbalboa/rfid/
	buffer[0] = PICC_CMD_SEL_CL1;
	buffer[1] = 0x20;

	rc52x_set_reg8(rc52x,MFRC_REG_ComlEnReg, 0x77 | 0x80);
	rc52x_and_reg8(rc52x,MFRC_REG_ComIrqReg,~0x80);
	rc52x_or_reg8(rc52x,MFRC_REG_FIFOLevelReg, 0x80);
	rc52x_set_reg8(rc52x,MFRC_REG_CommandReg, MFRC_Idle);

	mfrc522_send(rc52x,MFRC_REG_FIFODataReg,buffer, 2);
	rc52x_set_reg8(rc52x,MFRC_REG_CommandReg, MFRC_Transceive);

	rc52x_or_reg8(rc52x,MFRC_REG_BitFramingReg, 0x80);


	delay_ms(100);
	rc52x_and_reg8(rc52x,MFRC_REG_BitFramingReg, ~0x80);

	uint8_t error;
	rc52x_get_reg8(rc52x, MFRC_REG_ErrorReg, &error, 1);
	// Well... this is what the lib did, looks kinda weird
	// But for a quick test, let's go ahead. We'll need to look
	// at a proper implementation anyways...
	memset(buffer,0,10);

	if (! (error & 0x1B) ) {
		uint8_t size;
		rc52x_get_reg8(rc52x, MFRC_REG_FIFOLevelReg, &size, 1);
		if (size > 10) size = 10;
		mfrc522_recv(rc52x,MFRC_REG_FIFODataReg, buffer, size);
		return 0;
	}
	return -1;
}
