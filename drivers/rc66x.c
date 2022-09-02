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

void rc66x_antenna_on(rc66x_t *rc66x) {
	rc66x_or_reg8(rc66x, RC66X_REG_DrvMode, 0x03);

}
void rc66x_antenna_off(rc66x_t *rc66x) {
	rc66x_and_reg8(rc66x, RC66X_REG_DrvMode, ~0x03);
}

void rc66x_reset(rc66x_t *rc66x) {
	// TODO:  Extract reset pin from SPI struct
	//			 Current version in SPI struct stores polarity, use it

	// Note this one reset is active high!
	bshal_gpio_write_pin(rc66x->transport_instance.spim->rs_pin,
			rc66x->transport_instance.spim->rs_pol);
	rc66x->delay_ms(1);
	bshal_gpio_write_pin(rc66x->transport_instance.spim->rs_pin,
			!rc66x->transport_instance.spim->rs_pol);
}

void rc66x_init(rc66x_t *rc66x) {
	if (!rc66x)
		return;
	if (!rc66x->get_time_ms)
		return;
	if (!rc66x->delay_ms)
		return;
	rc66x->TransceiveData = rc66x_transceive;
	rc66x_reset(rc66x);

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

}

rc52x_result_t rc66x_transceive(rc66x_t *rc66x, uint8_t *sendData,
		uint8_t sendLen, uint8_t *backData, uint8_t *backLen,
		uint8_t *validBits, uint8_t rxAlign, uint8_t *collpos, bool sendCRC,
		bool recvCRC) {
	uint8_t waitIRq = 0b00010110;		// RxIRq and IdleIRq + ErrIRQ

	// Prepare values for BitFramingReg
	uint8_t txLastBits = validBits ? *validBits : 0;

	rc66x_set_reg8(rc66x, RC66X_REG_Command, RC66X_CMD_Idle);// Stop any active command.

	rc66x_set_reg8(rc66x, RC66X_REG_IRQ0, 0x7F);// Clear all seven interrupt request bits
	rc66x_set_reg8(rc66x, RC66X_REG_IRQ1, 0x7F);// Clear all seven interrupt request bits
	rc66x_set_reg8(rc66x, RC66X_REG_FIFOControl, 0xB0);	// FlushBuffer = 1, FIFO initialization
	rc66x_send(rc66x, RC66X_REG_FIFOData, sendData, sendLen);// Write sendData to the FIFO
	rc66x_set_reg8(rc66x, RC66X_REG_TxDataNum, 0x08 | txLastBits);
	rc66x_set_reg8(rc66x, RC66X_REG_RxBitCtrl, 0x80 | ((0x7 & rxAlign) << 4));

	rc66x_set_reg8(rc66x, RC66X_REG_TxCrcPreset, sendCRC ? 0x19 : 0x00);
	rc66x_set_reg8(rc66x, RC66X_REG_RxCrcPreset, recvCRC ? 0x19 : 0x00);

	rc66x_set_reg8(rc66x, RC66X_REG_Command, RC66X_CMD_Transceive);	// Execute the command

	uint32_t begin = rc66x->get_time_ms();

	while ((rc66x->get_time_ms() - begin) < 36) {
		uint8_t irq0, irq1;
		rc66x_get_reg8(rc66x, RC66X_REG_IRQ0, &irq0);
		rc66x_get_reg8(rc66x, RC66X_REG_IRQ1, &irq1);
		if (irq0 & waitIRq) {// One of the interrupts that signal success has been set.
			break;
		}
		if (irq1 & 0x01) {		// Timer interrupt - nothing received in 25ms
			return STATUS_TIMEOUT;
		}
	}

	// 35.7ms and nothing happend. Communication with the MFRC522 might be down.
	if ((rc66x->get_time_ms() - begin) >= RC66X_TIMEOUT_ms) {
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
		rc66x_get_reg8(rc66x, RC66X_REG_FIFOLength, &fifo_data_len);// Number of bytes in the FIFO

		if (fifo_data_len > *backLen) {
			return STATUS_NO_ROOM;
		}
		*backLen = fifo_data_len;					// Number of bytes returned
		rc66x_recv(rc66x, RC66X_REG_FIFOData, backData, fifo_data_len);

		rc66x_get_reg8(rc66x, RC66X_REG_RxBitCtrl, &_validBits);
		_validBits &= 0x07;	// RxLastBits[2:0] indicates the number of valid bits in the last received uint8_t. If this value is 000b, the whole uint8_t is valid.
		if (validBits) {
			*validBits = _validBits;
		}
	}

	// Tell about collisions
	if (errorRegValue & 0x04) {		// CollErr
		return STATUS_COLLISION;
	}

	// Does this still make sense?
	if (backData && backLen && recvCRC) {
		// In this case a MIFARE Classic NAK is not OK.
		if (*backLen == 1 && _validBits == 4) {
			return STATUS_MIFARE_NACK;
		}
		// We need at least the CRC_A value and all 8 bits of the last uint8_t must be received.
		if (*backLen < 2 || _validBits != 0) {
			return STATUS_CRC_WRONG;
		}
	}

	return STATUS_OK;
} // End RC52X_CommunicateWithPICC()
