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

int rc52x_get_chip_version(rc52x_t *rc52x, uint8_t *chip_id) {
	return mfrc522_recv(rc52x, MFRC_REG_VersionReg, chip_id, 1);
}

int rc52x_init(rc52x_t *rc52x) {
	// For initial testing, a sequence of commands found
	// in a working implementation
	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, MFRC_CMD_SoftReset);

	/*
	rc52x_set_reg8(rc52x, MFRC_REG_TModeReg, 0x8D);
	rc52x_set_reg8(rc52x, MFRC_REG_TPrescalerReg, 0x3E);
	rc52x_set_reg8(rc52x, MFRC_REG_TReloadReg_Lo, 30);
	rc52x_set_reg8(rc52x, MFRC_REG_TReloadReg_Hi, 0);
	rc52x_set_reg8(rc52x, MFRC_REG_RFCfgReg, 0x70);			// 48dB gain
	rc52x_set_reg8(rc52x, MFRC_REG_TxASKReg, 0x40);
	rc52x_set_reg8(rc52x, MFRC_REG_ModeReg, 0x3D);
*/
	rc52x_set_reg8(rc52x, MFRC_REG_TModeReg, 0x80);			// TAuto=1; timer starts automatically at the end of the transmission in all communication modes at all speeds
	rc52x_set_reg8(rc52x, MFRC_REG_TPrescalerReg, 0xA9);	// TPreScaler = TModeReg[3..0]:TPrescalerReg, ie 0x0A9 = 169 => f_timer=40kHz, ie a timer period of 25�s.
	rc52x_set_reg8(rc52x, MFRC_REG_TReloadReg_Hi, 0x03);		// Reload timer with 0x3E8 = 1000, ie 25ms before timeout.
	rc52x_set_reg8(rc52x, MFRC_REG_TReloadReg_Lo, 0xE8);
	rc52x_set_reg8(rc52x, MFRC_REG_TxASKReg, 0x40);		// Default 0x00. Force a 100 % ASK modulation independent of the ModGsPReg register setting
	rc52x_set_reg8(rc52x, MFRC_REG_ModeReg, 0x3D);		// Default 0x3F. Set the preset value for the CRC coprocessor for the CalcCRC command to 0x6363 (ISO 14443-3 part 6.2.4)


	rc52x_or_reg8(rc52x, MFRC_REG_TxControlReg, 0x03); // Antenna On
}

// Some defines from a known good implmementation
#define MI_OK			0
#define MI_NOTAGERR		1
#define MI_ERR			2
#define MFRC522_MAX_LEN					16				// Buf len byte

uint8_t MFRC522_ToCard(rc52x_t *rc52x, uint8_t command, uint8_t *sendData,
		uint8_t sendLen, uint8_t *backData, uint16_t *backLen) {

	uint8_t status = MI_ERR;
	uint8_t irqEn = 0x00;
	uint8_t waitIRq = 0x00;
	uint8_t lastBits;
	uint8_t n;
	uint16_t i;

	switch (command) {
	case MFRC_CMD_MFAuthent: {
		irqEn = 0x12;
		waitIRq = 0x10;
		break;
	}
	case MFRC_CMD_Transceive: {
		irqEn = 0x77;
		waitIRq = 0x30;
		break;
	}
	default:
		break;
	}

	rc52x_set_reg8(rc52x, MFRC_REG_ComlEnReg, irqEn | 0x80);
	rc52x_and_reg8(rc52x, MFRC_REG_ComIrqReg, ~0x80);
	//rc52x_set_reg8(rc52x,MFRC_REG_ComIrqReg,0x00);
	rc52x_or_reg8(rc52x, MFRC_REG_FIFOLevelReg, 0x80);
	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, MFRC_CMD_Idle);

	// Writing data to the FIFO
	mfrc522_send(rc52x, MFRC_REG_FIFODataReg, sendData, sendLen);

	// Execute the command
	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, command);
	if (command == MFRC_CMD_Transceive)
		rc52x_or_reg8(rc52x, MFRC_REG_BitFramingReg, 0x80);	// StartSend=1,transmission of data starts

	// Waiting to receive data to complete
	i = 2000;// i according to the clock frequency adjustment, the operator M1 card maximum waiting time 25ms
	do {
		// CommIrqReg[7..0]
		// Set1 TxIRq RxIRq IdleIRq HiAlerIRq LoAlertIRq ErrIRq TimerIRq
		//n = MFRC522_ReadRegister(MFRC522_REG_COMM_IRQ);
		rc52x_get_reg8(rc52x, MFRC_REG_ComIrqReg, &n);
		i--;
	} while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

	rc52x_and_reg8(rc52x, MFRC_REG_BitFramingReg, ~0x80);		// StartSend=0

	if (i != 0) {
		uint8_t error;
		rc52x_get_reg8(rc52x, MFRC_REG_ErrorReg, &error);
		if (!(error & 0x1B)) {
			status = MI_OK;
			if (n & irqEn & 0x01)
				status = MI_NOTAGERR;
			if (command == MFRC_CMD_Transceive) {

				rc52x_get_reg8(rc52x, MFRC_REG_FIFOLevelReg, &n);
				//lastBits = MFRC522_ReadRegister(MFRC522_REG_CONTROL) & 0x07;
				rc52x_get_reg8(rc52x, MFRC_REG_ControlReg, &lastBits);
				lastBits &= 0x07;

				if (lastBits)
					*backLen = (n - 1) * 8 + lastBits;
				else
					*backLen = n * 8;
				if (n == 0)
					n = 1;
				if (n > MFRC522_MAX_LEN)
					n = MFRC522_MAX_LEN;
				//for (i = 0; i < n; i++) backData[i] = MFRC522_ReadRegister(MFRC522_REG_FIFO_DATA);		// Reading the received data in FIFO
				mfrc522_recv(rc52x, MFRC_REG_FIFODataReg, backData, n);

			}
		} else
			status = MI_ERR;
	}
	return status;
}

int rc52x_reqa(rc52x_t *rc52x) {
	rc52x_and_reg8(rc52x, MFRC_REG_CollReg, ~0x80);

	rc52x_set_reg8(rc52x, MFRC_REG_BitFramingReg, 0x07);// TxLastBists = BitFramingReg[2..0]
	uint8_t buff[2] = {PICC_CMD_REQA};
	size_t size = 2;
	int status = 0;
	status = MFRC522_ToCard(rc52x, MFRC_CMD_Transceive, buff, 1, buff,
			&size);
	if ((status != MI_OK) || (size != 0x10))
		status = MI_ERR;

	if (status)
		return status;
}

int rc52x_anticol(rc52x_t *rc52x, uint8_t *serNum) {
	// FOr initial testing, function ported from known wokring implementaiton
	// However, we do not get past the REQA

	uint8_t status = MI_OK;
	uint8_t i;
	uint8_t serNumCheck = 0;
	uint16_t unLen;

	rc52x_set_reg8(rc52x, MFRC_REG_BitFramingReg, 0x07);// TxLastBists = BitFramingReg[2..0]
	serNum[0] = PICC_CMD_REQA;
	status = MFRC522_ToCard(rc52x, MFRC_CMD_Transceive, serNum, 1, serNum,
			&unLen);
	if ((status != MI_OK) || (unLen != 0x10))
		status = MI_ERR;

	if (status)
		return status;

	rc52x_set_reg8(rc52x, MFRC_REG_BitFramingReg, 0x00);
	serNum[0] = PICC_CMD_SEL_CL1;
	serNum[1] = 0x20;
	//status = MFRC522_ToCard(rc52x, MFRC_CMD_Transceive, serNum, 2, serNum, &unLen);
	/*
	 uint8_t waitIRq,///< The bits in the ComIrqReg register that signals successful completion of the command.
	 uint8_t *sendData,	///< Pointer to the data to transfer to the FIFO.
	 uint8_t sendLen,		///< Number of bytes to transfer to the FIFO.
	 uint8_t *backData,///< NULL or pointer to buffer if data should be read back after executing the command.
	 uint8_t *backLen,///< In: Max number of bytes to write to *backData. Out: The number of bytes returned.
	 uint8_t *validBits,	///< In/Out: The number of valid bits in the last byte. 0 for 8 valid bits.
	 uint8_t rxAlign ///< In: Defines the bit position in backData[0] for the first bit received. Default 0.
	 */
	uint8_t validBits = 0;
	status = rc52x_communicate_with_picc(rc52x, MFRC_CMD_Transceive, 0x30,
			serNum, 2, serNum, &unLen, &validBits, 0);

	if (status == MI_OK) {
		// Check card serial number
		//for (i = 0; i < 4; i++) serNumCheck ^= serNum[i];
		//if (serNumCheck != serNum[i]) status = MI_ERR;

		/*
		 int bytes = unLen / 8;
		 int bits = unLen % 8;
		 int dl = bytes;
		 if (bits) dl++;
		 for (i = dl; i > 1; i-- ) {
		 serNum[2+i] = serNum[i];
		 }
		 serNum[0] = PICC_CMD_SEL_CL1;
		 serNum[1] = bytes << 4 | bits;
		 status = MFRC522_ToCard(rc52x, MFRC_Transceive, serNum, 2+dl, serNum, &unLen);
		 */

	}
	return status;
}

// Ported "Unduino'd" from https://github.com/pkourany/MFRC522_RFID_Library
// Should have been https://github.com/miguelbalboa/rfid
uint8_t rc52x_communicate_with_picc(rc52x_t *rc52x, uint8_t command,///< The command to execute. One of the PCD_Command enums.
		uint8_t waitIRq,///< The bits in the ComIrqReg register that signals successful completion of the command.
		uint8_t *sendData,	///< Pointer to the data to transfer to the FIFO.
		uint8_t sendLen,		///< Number of bytes to transfer to the FIFO.
		uint8_t *backData,///< NULL or pointer to buffer if data should be read back after executing the command.
		uint8_t *backLen,///< In: Max number of bytes to write to *backData. Out: The number of bytes returned.
		uint8_t *validBits,	///< In/Out: The number of valid bits in the last byte. 0 for 8 valid bits.
		uint8_t rxAlign ///< In: Defines the bit position in backData[0] for the first bit received. Default 0.
		) {
	uint8_t n, _validBits;
	unsigned int i;

	// Prepare values for BitFramingReg
	uint8_t txLastBits = validBits ? *validBits : 0;
	uint8_t bitFraming = (rxAlign << 4) + txLastBits; // RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]

	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, MFRC_CMD_Idle); // Stop any active command.
	rc52x_set_reg8(rc52x, MFRC_REG_ComIrqReg, 0x7F); // Clear all seven interrupt request bits
	rc52x_or_reg8(rc52x, MFRC_REG_FIFOLevelReg, 0x80); // FlushBuffer = 1, FIFO initialization
	mfrc522_send(rc52x, MFRC_REG_FIFODataReg, sendData, sendLen);
	rc52x_set_reg8(rc52x, MFRC_REG_BitFramingReg, bitFraming); // Bit adjustments
	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, command); // Execute the command
	if (command == MFRC_CMD_Transceive) {
		rc52x_or_reg8(rc52x, MFRC_REG_BitFramingReg, 0x80);	// StartSend=1, transmission of data starts
	}

	// Wait for the command to complete.
	// In PCD_Init() we set the TAuto flag in TModeReg. This means the timer automatically starts when the PCD stops transmitting.
	// Each iteration of the do-while-loop takes 17.86µs.
	i = 2000;
	while (1) {
		rc52x_get_reg8(rc52x, MFRC_REG_ComIrqReg, &n); // ComIrqReg[7..0] bits are: Set1 TxIRq RxIRq IdleIRq   HiAlertIRq LoAlertIRq ErrIRq TimerIRq
		if (n & waitIRq) { // One of the interrupts that signal success has been set.
			break;
		}
		if (n & 0x01) {			// Timer interrupt - nothing received in 25ms
			return STATUS_TIMEOUT;

		}
		if (--i == 0) {	// The emergency break. If all other condions fail we will eventually terminate on this one after 35.7ms. Communication with the MFRC522 might be down.
			return STATUS_ERROR;
		}
	}

	// Stop now if any errors except collisions were detected.
	uint8_t errorRegValue;
	rc52x_get_reg8(rc52x, MFRC_REG_ErrorReg, &errorRegValue);
	// ErrorReg[7..0] bits are: WrErr TempErr reserved BufferOvfl   CollErr CRCErr ParityErr ProtocolErr
	if (errorRegValue & 0x13) {	 // BufferOvfl ParityErr ProtocolErr
		return STATUS_ERROR;
	}

	// If the caller wants data back, get it from the MFRC522.
	if (backData && backLen) {
		rc52x_get_reg8(rc52x, MFRC_REG_FIFOLevelReg, &n);
		//n = PCD_ReadRegister(FIFOLevelReg);						// Number of bytes in the FIFO
		if (n > *backLen) {
			return STATUS_NO_ROOM;
		}
		*backLen = n;								// Number of bytes returned
		mfrc522_recv(rc52x, MFRC_REG_FIFODataReg, backData, n);	// Get received data from FIFO

		// RxLastBits[2:0] indicates the number of valid bits in the last received byte. If this value is 000b, the whole byte is valid.
		rc52x_get_reg8(rc52x, MFRC_REG_ControlReg, &_validBits);
		_validBits &= 0x07;

		if (validBits) {
			*validBits = _validBits;
		}
	}

	// Tell about collisions
	if (errorRegValue & 0x08) { // CollErr
		return STATUS_COLLISION;
	}

	/*
	 // Perform CRC_A validation if requested.
	 if (backData && backLen && checkCRC) {
	 // In this case a MIFARE Classic NAK is not OK.
	 if (*backLen == 1 && _validBits == 4) {
	 return STATUS_MIFARE_NACK;
	 return -1;
	 }
	 // We need at least the CRC_A value and all 8 bits of the last byte must be received.
	 if (*backLen < 2 || _validBits != 0) {
	 return STATUS_CRC_WRONG;
	 }
	 // Verify CRC_A - do our own calculation and store the control in controlBuffer.
	 byte controlBuffer[2];
	 n = PCD_CalculateCRC(&backData[0], *backLen - 2, &controlBuffer[0]);
	 if (n != STATUS_OK) {
	 return n;
	 }
	 if ((backData[*backLen - 2] != controlBuffer[0]) || (backData[*backLen - 1] != controlBuffer[1])) {
	 return STATUS_CRC_WRONG;
	 }
	 }
	 */
	return STATUS_OK;
} // End PCD_CommunicateWithPICC()


//--
uint8_t _PCD_CalculateCRC(rc52x_t*rc52x,	uint8_t *data,		///< In: Pointer to the data to transfer to the FIFO for CRC calculation.
		uint8_t length,	///< In: The number of bytes to transfer.
		uint8_t *result	///< Out: Pointer to result buffer. Result is written to result[0..1], low byte first.
					 ) {
	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, MFRC_CMD_Idle);			// Stop any active command.
	rc52x_set_reg8(rc52x, MFRC_REG_DivIrqReg, 0x04);					// Clear the CRCIRq interrupt request bit
	rc52x_or_reg8(rc52x, MFRC_REG_FIFOLevelReg, 0x80);		// FlushBuffer = 1, FIFO initialization


	mfrc522_send(rc52x, MFRC_REG_FIFODataReg, data, length);// Write data to the FIFO

	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, MFRC_CMD_CalcCRC);		// Start the calculation

	// Wait for the CRC calculation to complete. Each iteration of the while-loop takes 17.73�s.
	uint16_t i = 5000;
	uint8_t n;
	while (1) {
		//n = PCD_ReadRegister(DivIrqReg);	// DivIrqReg[7..0] bits are: Set2 reserved reserved MfinActIRq   reserved CRCIRq reserved reserved
		rc52x_get_reg8(rc52x, MFRC_REG_DivIrqReg, &n);
		if (n & 0x04) {						// CRCIRq bit set - calculation done
			break;
		}
		if (--i == 0) {						// The emergency break. We will eventually terminate on this one after 89ms. Communication with the MFRC522 might be down.
			return STATUS_TIMEOUT;
		}
	}
	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, MFRC_CMD_Idle);			// Stop calculating CRC for new content in the FIFO.

	// Transfer the result from the registers to the result buffer
	/*
	result[0] = PCD_ReadRegister(CRCResultRegL);
	result[1] = PCD_ReadRegister(CRCResultRegH);
	*/
	rc52x_get_reg8(rc52x, MFRC_REG_CRCResultReg_Lo, &result[0]);
	rc52x_get_reg8(rc52x, MFRC_REG_CRCResultReg_Hi, &result[1]);
	return STATUS_OK;
} // End PCD_CalculateCRC()
//--




// Ported "Unduino'd" from https://github.com/pkourany/MFRC522_RFID_Library
uint8_t _PICC_Select(rc52x_t *rc52x, Uid *uid,///< Pointer to Uid struct. Normally output, but can also be used to supply a known UID.
		uint8_t validBits///< The number of known UID bits supplied in *uid. Normally 0. If set you must also supply uid->size.
		) {
	bool uidComplete;
	bool selectDone;
	bool useCascadeTag;
	uint8_t cascadeLevel = 1;
	rc52x_result_t result;
	uint8_t count;
	uint8_t checkBit;
	uint8_t index;
	uint8_t uidIndex;// The first index in uid->uidByte[] that is used in the current Cascade Level.
	int8_t currentLevelKnownBits;// The number of known UID bits in the current Cascade Level.
	uint8_t buffer[9];// The SELECT/ANTICOLLISION commands uses a 7 byte standard frame + 2 bytes CRC_A
	uint8_t bufferUsed;	// The number of bytes used in the buffer, ie the number of bytes to transfer to the FIFO.
	uint8_t rxAlign;// Used in BitFramingReg. Defines the bit position for the first bit received.
	uint8_t txLastBits;	// Used in BitFramingReg. The number of valid bits in the last transmitted byte.
	uint8_t *responseBuffer;
	uint8_t responseLength;

	// Description of buffer structure:
	// 		Byte 0: SEL 				Indicates the Cascade Level: PICC_CMD_SEL_CL1, PICC_CMD_SEL_CL2 or PICC_CMD_SEL_CL3
	// 		Byte 1: NVB					Number of Valid Bits (in complete command, not just the UID): High nibble: complete bytes, Low nibble: Extra bits.
	// 		Byte 2: UID-data or CT		See explanation below. CT means Cascade Tag.
	// 		Byte 3: UID-data
	// 		Byte 4: UID-data
	// 		Byte 5: UID-data
	// 		Byte 6: BCC					Block Check Character - XOR of bytes 2-5
	//		Byte 7: CRC_A
	//		Byte 8: CRC_A
	// The BCC and CRC_A is only transmitted if we know all the UID bits of the current Cascade Level.
	//
	// Description of bytes 2-5: (Section 6.5.4 of the ISO/IEC 14443-3 draft: UID contents and cascade levels)
	//		UID size	Cascade level	Byte2	Byte3	Byte4	Byte5
	//		========	=============	=====	=====	=====	=====
	//		 4 bytes		1			uid0	uid1	uid2	uid3
	//		 7 bytes		1			CT		uid0	uid1	uid2
	//						2			uid3	uid4	uid5	uid6
	//		10 bytes		1			CT		uid0	uid1	uid2
	//						2			CT		uid3	uid4	uid5
	//						3			uid6	uid7	uid8	uid9

	// Sanity checks
	if (validBits > 80) {
		return STATUS_INVALID;
	}

	// Prepare MFRC522
	//PCD_ClearRegisterBitMask(CollReg, 0x80);// ValuesAfterColl=1 => Bits received after collision are cleared.
	rc52x_and_reg8(rc52x, MFRC_REG_CollReg, ~0x80);// ValuesAfterColl=1 => Bits received after collision are cleared.

	// Repeat Cascade Level loop until we have a complete UID.
	uidComplete = false;
	while (!uidComplete) {
		// Set the Cascade Level in the SEL byte, find out if we need to use the Cascade Tag in byte 2.
		switch (cascadeLevel) {
		case 1:
			buffer[0] = PICC_CMD_SEL_CL1;
			uidIndex = 0;
			useCascadeTag = validBits && uid->size > 4;	// When we know that the UID has more than 4 bytes
			break;

		case 2:
			buffer[0] = PICC_CMD_SEL_CL2;
			uidIndex = 3;
			useCascadeTag = validBits && uid->size > 7;	// When we know that the UID has more than 7 bytes
			break;

		case 3:
			buffer[0] = PICC_CMD_SEL_CL3;
			uidIndex = 6;
			useCascadeTag = false;						// Never used in CL3.
			break;

		default:
			return STATUS_INTERNAL_ERROR;
			break;
		}

		// How many UID bits are known in this Cascade Level?
		currentLevelKnownBits = validBits - (8 * uidIndex);
		if (currentLevelKnownBits < 0) {
			currentLevelKnownBits = 0;
		}
		// Copy the known bits from uid->uidByte[] to buffer[]
		index = 2; // destination index in buffer[]
		if (useCascadeTag) {
			buffer[index++] = PICC_CMD_CT;
		}
		uint8_t bytesToCopy = currentLevelKnownBits / 8
				+ (currentLevelKnownBits % 8 ? 1 : 0); // The number of bytes needed to represent the known bits for this level.
		if (bytesToCopy) {
			uint8_t maxBytes = useCascadeTag ? 3 : 4; // Max 4 bytes in each Cascade Level. Only 3 left if we use the Cascade Tag
			if (bytesToCopy > maxBytes) {
				bytesToCopy = maxBytes;
			}
			for (count = 0; count < bytesToCopy; count++) {
				buffer[index++] = uid->uidByte[uidIndex + count];
			}
		}
		// Now that the data has been copied we need to include the 8 bits in CT in currentLevelKnownBits
		if (useCascadeTag) {
			currentLevelKnownBits += 8;
		}

		// Repeat anti collision loop until we can transmit all UID bits + BCC and receive a SAK - max 32 iterations.
		selectDone = false;
		while (!selectDone) {
			// Find out how many bits and bytes to send and receive.
			if (currentLevelKnownBits >= 32) { // All UID bits in this Cascade Level are known. This is a SELECT.
				//Serial.print("SELECT: currentLevelKnownBits="); Serial.println(currentLevelKnownBits, DEC);
				buffer[1] = 0x70; // NVB - Number of Valid Bits: Seven whole bytes
				// Calulate BCC - Block Check Character
				buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];



				// TODO
				// Calculate CRC_A
				result = _PCD_CalculateCRC(rc52x, buffer, 7, &buffer[7]);
				if (result != STATUS_OK) {
					return result;
				}



				txLastBits = 0; // 0 => All 8 bits are valid.
				bufferUsed = 9;
				// Store response in the last 3 bytes of buffer (BCC and CRC_A - not needed after tx)
				responseBuffer = &buffer[6];
				responseLength = 3;
			} else { // This is an ANTICOLLISION.
					 //Serial.print("ANTICOLLISION: currentLevelKnownBits="); Serial.println(currentLevelKnownBits, DEC);
				txLastBits = currentLevelKnownBits % 8;
				count = currentLevelKnownBits / 8;// Number of whole bytes in the UID part.
				index = 2 + count;	// Number of whole bytes: SEL + NVB + UIDs
				buffer[1] = (index << 4) + txLastBits;// NVB - Number of Valid Bits
				bufferUsed = index + (txLastBits ? 1 : 0);
				// Store response in the unused part of buffer
				responseBuffer = &buffer[index];
				responseLength = sizeof(buffer) - index;
			}

			// Set bit adjustments
			rxAlign = txLastBits;// Having a seperate variable is overkill. But it makes the next line easier to read.




			//PCD_WriteRegister(BitFramingReg, (rxAlign << 4) + txLastBits);// RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]
			// This seems redundant as it is done in communicate with picc?
			//rc52x_set_reg8(rc52x, MFRC_REG_BitFramingReg, (rxAlign << 4) + txLastBits);



			// Transmit the buffer and receive the response.
			result = rc52x_communicate_with_picc(rc52x, MFRC_CMD_Transceive, 0x30, buffer, bufferUsed, responseBuffer,
					&responseLength, &txLastBits, rxAlign);

			if (result == STATUS_COLLISION) { // More than one PICC in the field => collision.
				//result = PCD_ReadRegister(CollReg); // CollReg[7..0] bits are: ValuesAfterColl reserved CollPosNotValid CollPos[4:0]
				uint8_t valueOfCollReg;
				rc52x_get_reg8(rc52x, MFRC_REG_CollReg,&valueOfCollReg);
				if (valueOfCollReg & 0x20) { // CollPosNotValid
					return STATUS_COLLISION; // Without a valid collision position we cannot continue
				}
				uint8_t collisionPos = result & 0x1F; // Values 0-31, 0 means bit 32.
				if (collisionPos == 0) {
					collisionPos = 32;
				}
				if (collisionPos <= currentLevelKnownBits) { // No progress - should not happen
					return STATUS_INTERNAL_ERROR;
				}
				// Choose the PICC with the bit set.
				currentLevelKnownBits	= collisionPos;
				count			= currentLevelKnownBits % 8; // The bit to modify
				checkBit		= (currentLevelKnownBits - 1) % 8;
				index			= 1 + (currentLevelKnownBits / 8) + (count ? 1 : 0); // First byte is index 0.
				buffer[index]	|= (1 << checkBit);

			} else if (result != STATUS_OK) {
				return result;
			} else { // STATUS_OK
				if (currentLevelKnownBits >= 32) { // This was a SELECT.
					selectDone = true; // No more anticollision
					// We continue below outside the while.
				} else { // This was an ANTICOLLISION.
						 // We now have all 32 bits of the UID in this Cascade Level
					currentLevelKnownBits = 32;
					// Run loop again to do the SELECT.
				}
			}
		} // End of while ( ! selectDone)

		// We do not check the CBB - it was constructed by us above.

		// Copy the found UID bytes from buffer[] to uid->uidByte[]
		index = (buffer[2] == PICC_CMD_CT) ? 3 : 2; // source index in buffer[]
		bytesToCopy = (buffer[2] == PICC_CMD_CT) ? 3 : 4;
		for (count = 0; count < bytesToCopy; count++) {
			uid->uidByte[uidIndex + count] = buffer[index++];
		}

		// Check response SAK (Select Acknowledge)
		if (responseLength != 3 || txLastBits != 0) {// SAK must be exactly 24 bits (1 byte + CRC_A).
			return STATUS_ERROR;
		}


		 // TODO
		// Verify CRC_A - do our own calculation and store the control in buffer[2..3] - those bytes are not needed anymore.
		result = PCD_CalculateCRC(rc52x, responseBuffer, 1, &buffer[2]);
		if (result != STATUS_OK) {
			return result;
		}
		/*

		if ((buffer[2] != responseBuffer[1])
				|| (buffer[3] != responseBuffer[2])) {
			return STATUS_CRC_WRONG;
		}
		*/

		if (responseBuffer[0] & 0x04) { // Cascade bit set - UID not complete yes
			cascadeLevel++;
		} else {
			uidComplete = true;
			uid->sak = responseBuffer[0];
		}
	} // End of while ( ! uidComplete)

	// Set correct uid->size
	uid->size = 3 * cascadeLevel + 1;

	return STATUS_OK;
} // End PICC_Select()

