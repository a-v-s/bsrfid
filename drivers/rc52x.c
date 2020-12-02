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
	// in a working implementation
	//rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, MFRC_SoftReset);
	HAL_Delay(100);

	rc52x_set_reg8(rc52x, MFRC_REG_TModeReg, 0x8D);
	rc52x_set_reg8(rc52x, MFRC_REG_TPrescalerReg, 0x3E);
	rc52x_set_reg8(rc52x, MFRC_REG_TReloadReg_Lo, 30);
	rc52x_set_reg8(rc52x, MFRC_REG_TReloadReg_Hi, 0);
	rc52x_set_reg8(rc52x, MFRC_REG_RFCfgReg, 0x70);			// 48dB gain
	rc52x_set_reg8(rc52x, MFRC_REG_TxASKReg, 0x40);
	rc52x_set_reg8(rc52x, MFRC_REG_ModeReg, 0x3D);


	rc52x_or_reg8(rc52x, MFRC_REG_TxControlReg, 0x03); // Antenna On
}


// Some defines from a known good implmementation
#define MI_OK			0
#define MI_NOTAGERR		1
#define MI_ERR			2
#define MFRC522_MAX_LEN					16				// Buf len byte

uint8_t MFRC522_ToCard(rc52x_t *rc52x, uint8_t command, uint8_t * sendData, uint8_t sendLen, uint8_t * backData, uint16_t * backLen) {


	uint8_t status = MI_ERR;
	uint8_t irqEn = 0x00;
	uint8_t waitIRq = 0x00;
	uint8_t lastBits;
	uint8_t n;
	uint16_t i;

	switch (command) {
		case MFRC_MFAuthent: {
			irqEn = 0x12;
			waitIRq = 0x10;
			break;
		}
		case MFRC_Transceive: {
			irqEn = 0x77;
			waitIRq = 0x30;
			break;
		}
		default:
		break;
	}

	rc52x_set_reg8(rc52x,MFRC_REG_ComlEnReg,  irqEn | 0x80);
	//rc52x_and_reg8(rc52x,MFRC_REG_ComIrqReg, ~0x80);
	rc52x_set_reg8(rc52x,MFRC_REG_ComIrqReg,0x00);
	rc52x_or_reg8(rc52x,MFRC_REG_FIFOLevelReg,  0x80);
	rc52x_set_reg8(rc52x,MFRC_REG_CommandReg, MFRC_Idle);

	// Writing data to the FIFO
	mfrc522_send(rc52x,MFRC_REG_FIFODataReg,sendData, sendLen);

	// Execute the command
	rc52x_set_reg8(rc52x,MFRC_REG_CommandReg, command);
	if (command == MFRC_Transceive)
		rc52x_or_reg8(rc52x,MFRC_REG_BitFramingReg, 0x80);					// StartSend=1,transmission of data starts

	// Waiting to receive data to complete
	i = 2000;	// i according to the clock frequency adjustment, the operator M1 card maximum waiting time 25ms
	do {
		// CommIrqReg[7..0]
		// Set1 TxIRq RxIRq IdleIRq HiAlerIRq LoAlertIRq ErrIRq TimerIRq
		//n = MFRC522_ReadRegister(MFRC522_REG_COMM_IRQ);
		rc52x_get_reg8(rc52x,MFRC_REG_ComIrqReg, &n);
		i--;
	} while ((i!=0) && !(n&0x01) && !(n&waitIRq));

	rc52x_and_reg8(rc52x,MFRC_REG_BitFramingReg, ~0x80);												// StartSend=0

	if (i != 0)  {
		uint8_t error;
		rc52x_get_reg8(rc52x,MFRC_REG_ErrorReg, &error);
		if (!(error & 0x1B)) {
			status = MI_OK;
			if (n & irqEn & 0x01) status = MI_NOTAGERR;
			if (command == MFRC_Transceive) {
				//n = MFRC522_ReadRegister(MFRC522_REG_FIFO_LEVEL);
				rc52x_get_reg8(rc52x,MFRC_REG_FIFOLevelReg, &n);
				//lastBits = MFRC522_ReadRegister(MFRC522_REG_CONTROL) & 0x07;
				rc52x_get_reg8(rc52x,MFRC_REG_ControlReg, &lastBits);
				lastBits &= 0x07;

				if (lastBits) *backLen = (n - 1) * 8 + lastBits; else *backLen = n * 8;
				if (n == 0) n = 1;
				if (n > MFRC522_MAX_LEN) n = MFRC522_MAX_LEN;
				//for (i = 0; i < n; i++) backData[i] = MFRC522_ReadRegister(MFRC522_REG_FIFO_DATA);		// Reading the received data in FIFO
				mfrc522_recv(rc52x,MFRC_REG_FIFODataReg, backData, n);

			}
		} else status = MI_ERR;
	}
	return status;
}

int rc52x_anticol(rc52x_t *rc52x, uint8_t * serNum) {
	// FOr initial testing, function ported from known wokring implementaiton
	// However, we do not get past the REQA

	uint8_t status = MI_OK;
	uint8_t i;
	uint8_t serNumCheck = 0;
	uint16_t unLen;

	rc52x_set_reg8(rc52x,MFRC_REG_BitFramingReg, 0x07);												// TxLastBists = BitFramingReg[2..0]
	serNum[0] = PICC_CMD_REQA;
	status = MFRC522_ToCard(rc52x, MFRC_Transceive, serNum, 1, serNum, &unLen);
	if ((status != MI_OK) || (unLen != 0x10)) status = MI_ERR;

	if (status) return status;

	rc52x_set_reg8(rc52x,MFRC_REG_BitFramingReg, 0x00);
	serNum[0] = PICC_CMD_SEL_CL1;
	serNum[1] = 0x20;
	status = MFRC522_ToCard(rc52x, MFRC_Transceive, serNum, 2, serNum, &unLen);
	if (status == MI_OK) {
		// Check card serial number
		for (i = 0; i < 4; i++) serNumCheck ^= serNum[i];
		if (serNumCheck != serNum[i]) status = MI_ERR;
	}
	return status;
}
