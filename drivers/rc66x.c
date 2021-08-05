/*
 * rc66x.c
 *
 *  Created on: 28 dec. 2020
 *      Author: andre
 */


#include "rc66x.h"

#include "bshal_spim.h"

// expecting 0x18 or 0x1A
int rc66x_get_chip_version(rc66x_t *rc66x, uint8_t *chip_id) {
	return rc66x_recv(rc66x, RC66X_REG_Version, chip_id, 1);
}


void RC66X_AntennaOn(rc66x_t *rc66x){
	rc66x_or_reg8(rc66x,RC66X_REG_DrvMode,0x03);

}
void RC66X_AntennaOff(rc66x_t *rc66x){
	rc66x_and_reg8(rc66x,RC66X_REG_DrvMode,~0x03);
}

void RC66X_Reset(rc66x_t *rc66x) {
	// Note this one reset is active high!
	bshal_gpio_write_pin(((bshal_spim_t*)(rc66x->transport_config))->nrs_pin, 1);
	HAL_Delay(1);
	bshal_gpio_write_pin(((bshal_spim_t*)(rc66x->transport_config))->nrs_pin, 0);
}

void RC66X_Init(rc66x_t *rc66x) {
	rc66x->TransceiveData = RC66X_TransceiveData;
	RC66X_Reset(rc66x);

	/*
	 // Reset baud rates
	 rc52x_set_reg8(rc52x, RC52X_REG_TxModeReg, 0x00);
	 rc52x_set_reg8(rc52x, RC52X_REG_RxModeReg, 0x00);
	 // Reset ModWidthReg
	 rc52x_set_reg8(rc52x, RC52X_REG_ModWidthReg, 0x26);

	 // When communicating with a PICC we need a timeout if something goes wrong.
	 // f_timer = 13.56 MHz / (2*TPreScaler+1) where TPreScaler = [TPrescaler_Hi:TPrescaler_Lo].
	 // TPrescaler_Hi are the four low bits in TModeReg. TPrescaler_Lo is TPrescalerReg.
	 rc52x_set_reg8(rc52x, RC52X_REG_TModeReg, 0x80);	// TAuto=1; timer starts automatically at the end of the transmission in all communication modes at all speeds
	 rc52x_set_reg8(rc52x, RC52X_REG_TPrescalerReg, 0xA9);// TPreScaler = TModeReg[3..0]:TPrescalerReg, ie 0x0A9 = 169 => f_timer=40kHz, ie a timer period of 25μs.
	 rc52x_set_reg8(rc52x, RC52X_REG_TReloadReg_Hi, 0x03);// Reload timer with 0x3E8 = 1000, ie 25ms before timeout.
	 rc52x_set_reg8(rc52x, RC52X_REG_TReloadReg_Lo, 0xE8);

	 rc52x_set_reg8(rc52x, RC52X_REG_TxASKReg, 0x40);	// Default 0x00. Force a 100 % ASK modulation independent of the ModGsPReg register setting
	 rc52x_set_reg8(rc52x, RC52X_REG_ModeReg, 0x3D);// Default 0x3F. Set the preset value for the CRC coprocessor for the CalcCRC command to 0x6363 (ISO 14443-3 part 6.2.4)
	 */

	// Translated from AN12657  4.1.1
	// 1. Cancels previous executions and the state machine returns into IDLE mode
	rc66x_set_reg8(rc66x, RC66X_REG_Command, RC66X_CMD_Idle);

	// 2. Flushes the FIFO and defines FIFO characteristics
	rc66x_set_reg8(rc66x, RC66X_REG_FIFOControl, 0xB0);

	// 3. Fills the FIFO with 0x00 and 0x00.
	uint8_t load_protocol_parameters[] = { 0x00, 0x00 };
	rc66x_send(rc66x, RC66X_REG_FIFOData, load_protocol_parameters,
			sizeof(load_protocol_parameters));

	// 4. Executes LoadProtocol command with parameters 0x00 and 0x00 (FIFO).
	// This	translates to load protocol ISO14443A - 106
	rc66x_set_reg8(rc66x, RC66X_REG_Command, RC66X_CMD_LoadProtocol);

	// 5. Flushes the FIFO and defines FIFO characteristics
	rc66x_set_reg8(rc66x, RC66X_REG_FIFOControl, 0xB0);

	// 6. Switches the RF filed ON.
	rc66x_set_reg8(rc66x, RC66X_REG_DrvMode, 0x8E);

	// 7. Clears all bits in IRQ0
	rc66x_set_reg8(rc66x, RC66X_REG_IRQ0, 0x7F);

	// 8. Switches the CRC extention OFF in tx direction
	rc66x_set_reg8(rc66x, RC66X_REG_TxCrcPreset, 0x18);

	// 9. Switches the CRC extention OFF in rx direction
	rc66x_set_reg8(rc66x, RC66X_REG_RxCrcPreset, 0x18);

	// The rest will go to the communicate with picc stuff

} // End RC52X_Init()




/**
 * Executes the Transceive command.
 * CRC validation can only be done if backData and backLen are specified.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t RC66X_TransceiveData(rc66x_t *rc66x,uint8_t *sendData,
		uint8_t sendLen,
		uint8_t *backData,
		uint8_t *backLen,
		uint8_t *validBits,
		uint8_t rxAlign,
		uint8_t * collpos,
		bool sendCRC ,
		bool recvCRC
		) {
	uint8_t waitIRq = 0b00010110;		// RxIRq and IdleIRq + ErrIRQ
	return RC66X_CommunicateWithPICC(rc66x, RC66X_CMD_Transceive, waitIRq,
			sendData, sendLen, backData, backLen, validBits, rxAlign,  sendCRC, recvCRC);
} // End RC52X_TransceiveData()

/**
 * Transfers data to the MFRC522 FIFO, executes a command, waits for completion and transfers data back from the FIFO.
 * CRC validation can only be done if backData and backLen are specified.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t RC66X_CommunicateWithPICC(rc66x_t *rc66x, uint8_t command,	///< The command to execute. One of the RC52X_Command enums.
		uint8_t waitIRq,///< The bits in the ComIrqReg register that signals successful completion of the command.
		uint8_t *sendData,	///< Pointer to the data to transfer to the FIFO.
		uint8_t sendLen,		///< Number of uint8_ts to transfer to the FIFO.
		uint8_t *backData,///< nullptr or pointer to buffer if data should be read back after executing the command.
		uint8_t *backLen,///< In: Max number of uint8_ts to write to *backData. Out: The number of uint8_ts returned.
		uint8_t *validBits,	///< In/Out: The number of valid bits in the last uint8_t. 0 for 8 valid bits.
		uint8_t rxAlign,///< In: Defines the bit position in backData[0] for the first bit received. Default 0.
		bool sendCRC ,
		bool recvCRC
		) {
	// Prepare values for BitFramingReg
	uint8_t txLastBits = validBits ? *validBits : 0;

	rc66x_set_reg8(rc66x, RC66X_REG_Command, RC66X_CMD_Idle);// Stop any active command.

	rc66x_set_reg8(rc66x, RC66X_REG_IRQ0, 0x7F);// Clear all seven interrupt request bits
	rc66x_set_reg8(rc66x, RC66X_REG_IRQ1, 0x7F);// Clear all seven interrupt request bits
	rc66x_set_reg8(rc66x, RC66X_REG_FIFOControl, 0xB0);	// FlushBuffer = 1, FIFO initialization
	rc66x_send(rc66x, RC66X_REG_FIFOData, sendData, sendLen);// Write sendData to the FIFO
	rc66x_set_reg8(rc66x, RC66X_REG_TxDataNum,  0x08 | txLastBits);
	rc66x_set_reg8(rc66x, RC66X_REG_RxBitCtrl, 0x80 | ((0x7&rxAlign)<<4));

	rc66x_set_reg8(rc66x, RC66X_REG_TxCrcPreset, sendCRC ? 0x19 : 0x00);
	rc66x_set_reg8(rc66x, RC66X_REG_RxCrcPreset, recvCRC? 0x19 : 0x00);

	rc66x_set_reg8(rc66x, RC66X_REG_Command,  command);// Execute the command


	// Wait for the command to complete.
	// In RC52X_Init() we set the TAuto flag in TModeReg. This means the timer automatically starts when the PCD stops transmitting.
	// Each iteration of the do-while-loop takes 17.86μs.
	// TODO check/modify for other architectures than Arduino Uno 16bit
	uint16_t i;
	for (i = 2000; i > 0; i--) {
		uint8_t irq0, irq1;
		rc66x_get_reg8(rc66x, RC66X_REG_IRQ0, &irq0);
		rc66x_get_reg8(rc66x, RC66X_REG_IRQ1, &irq1);
		if (irq0 & waitIRq) {// One of the interrupts that signal success has been set.
			break;
		}
		if (irq1 & 0x01) {			// Timer interrupt - nothing received in 25ms
			return STATUS_TIMEOUT;
		}
	}
	// 35.7ms and nothing happend. Communication with the MFRC522 might be down.
	if (i == 0) {
		return STATUS_TIMEOUT;
	}

	// Stop now if any errors except collisions were detected.
	uint8_t errorRegValue;
			rc66x_get_reg8(rc66x, RC66X_REG_Error, &errorRegValue);
	if (errorRegValue & 0b01100011) {	 // BufferOvfl ParityErr ProtocolErr
		return STATUS_ERROR;
	}

	uint8_t _validBits = 0;

	// If the caller wants data back, get it from the MFRC522.
	if (backData && backLen) {
		uint8_t fifo_data_len;
		rc66x_get_reg8(rc66x, RC66X_REG_FIFOLength,&fifo_data_len);// Number of uint8_ts in the FIFO

		if (fifo_data_len > *backLen) {
			return STATUS_NO_ROOM;
		}
		*backLen = fifo_data_len;							// Number of bytes returned
		rc66x_recv(rc66x, RC66X_REG_FIFOData, backData, fifo_data_len);

		rc66x_get_reg8(rc66x, RC66X_REG_RxBitCtrl, &_validBits);
		_validBits &= 0x07;// RxLastBits[2:0] indicates the number of valid bits in the last received uint8_t. If this value is 000b, the whole uint8_t is valid.
		if (validBits) {
			*validBits = _validBits;
		}
	}

	// Tell about collisions
	if (errorRegValue & 0x04) {		// CollErr
		return STATUS_COLLISION;
	}

	// Perform CRC_A validation if requested.
	if (backData && backLen && recvCRC) {
		// In this case a MIFARE Classic NAK is not OK.
		if (*backLen == 1 && _validBits == 4) {
			return STATUS_MIFARE_NACK;
		}
		// We need at least the CRC_A value and all 8 bits of the last uint8_t must be received.
		if (*backLen < 2 || _validBits != 0) {
			return STATUS_CRC_WRONG;
		}
		/*

		// Verify CRC_A - do our own calculation and store the control in controlBuffer.
		uint8_t controlBuffer[2];
		rc52x_result_t status = RC52X_CalculateCRC(rc52x, &backData[0],
				*backLen - 2, &controlBuffer[0]);
		if (status != STATUS_OK) {
			return status;
		}
		if ((backData[*backLen - 2] != controlBuffer[0])
				|| (backData[*backLen - 1] != controlBuffer[1])) {
			return STATUS_CRC_WRONG;
		}

		*/
	}

	return STATUS_OK;
} // End RC52X_CommunicateWithPICC()
