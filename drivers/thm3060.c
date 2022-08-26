/*
 * thm3060.c
 *
 *  Created on: 11 aug. 2022
 *      Author: andre
 */

#include "thm3060.h"



int thm3060_recv(thm3060_t *thm3060, uint8_t reg, uint8_t *data, size_t amount) {
	int result = 0;
	reg &=~THM3060_DIR_WRITE;
	result = bshal_spim_transmit(thm3060->transport_instance.spim, &reg, 1, true);
	if (result) return result;
	return bshal_spim_receive(thm3060->transport_instance.spim, data, amount, false);
}

int thm3060_send(thm3060_t *thm3060, uint8_t reg, uint8_t *data, size_t amount) {
	int result = 0;
	reg |= THM3060_DIR_WRITE;
	result = bshal_spim_transmit(thm3060->transport_instance.spim, &reg, 1, true);
	if (result) return result;
	return bshal_spim_transmit(thm3060->transport_instance.spim, data, amount, false);
}

int thm3060_get_reg8(thm3060_t *thm3060, uint8_t reg, uint8_t *value) {
	return thm3060_recv(thm3060, reg , value, 1);
}

int thm3060_set_reg8(thm3060_t *thm3060, uint8_t reg, uint8_t value) {
	return thm3060_send(thm3060, reg , &value, 1);
}

int thm3060_or_reg8(thm3060_t *thm3060, uint8_t reg, uint8_t value) {
	uint8_t tmpval;
	int result;
	result = thm3060_get_reg8(thm3060, reg, &tmpval);
	if (result)
		return result;
	tmpval |= value;
	return thm3060_set_reg8(thm3060, reg, tmpval);
}

int thm3060_and_reg8(thm3060_t *thm3060, uint8_t reg, uint8_t value) {
	uint8_t tmpval;
	int result;
	result = thm3060_get_reg8(thm3060, reg, &tmpval);
	if (result)
		return result;
	tmpval &= value;
	return thm3060_set_reg8(thm3060, reg, tmpval);
}



void THM3060_AntennaOn(thm3060_t *thm3060){
	thm3060_or_reg8(thm3060, THM3060_REG_SCON, 0x01);

}
void THM3060_AntennaOff(thm3060_t *thm3060){
	thm3060_and_reg8(thm3060, THM3060_REG_SCON, ~0x01);
}




rc52x_result_t THM3060_CommunicateWithPICC(thm3060_t *thm3060, uint8_t command,	///< The command to execute. One of the RC52X_Command enums.
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

	// Is there a "go to idle" command

	thm3060_set_reg8(thm3060, THM3060_REG_RSTAT, 0x00);	// Clear interrupts


	thm3060_or_reg8(thm3060, THM3060_REG_SCON, 0x04);	// Clear buffer
	thm3060_and_reg8(thm3060, THM3060_REG_SCON, ~0x04);	// Ready buffer,
	thm3060_send(thm3060, THM3060_REG_DATA, sendData, sendLen);// Write sendData to the FIFO

	thm3060_set_reg8(thm3060, THM3060_REG_RSCL, sendLen);
	thm3060_set_reg8(thm3060, THM3060_REG_RSCH, 0);
	//thm3060_set_reg8(thm3060, THM3060_REG_BPOS, txLastBits);

	//--------

	if (sendCRC) {
		thm3060_or_reg8(thm3060,THM3060_REG_CRCSEL, 0x80);
	} else {
		thm3060_and_reg8(thm3060,THM3060_REG_CRCSEL, ~0x80);
	}

	if (recvCRC) {
		thm3060_or_reg8(thm3060,THM3060_REG_CRCSEL, 0x40);
	} else {
		thm3060_and_reg8(thm3060,THM3060_REG_CRCSEL, ~0x40);
	}

	thm3060_or_reg8(thm3060,THM3060_REG_SCON, 0x02); // start

	bshal_delay_ms(5);

	// Wait for the command to complete.
	// In RC52X_Init() we set the TAuto flag in TModeReg. This means the timer automatically starts when the PCD stops transmitting.
	// Each iteration of the do-while-loop takes 17.86μs.
	// TODO check/modify for other architectures than Arduino Uno 16bit

	uint32_t begin = HAL_GetTick(); // TODO replace with get_time_ms();

	bool collision = false;

	while ((HAL_GetTick() - begin) < 36) {
		uint8_t irq;
		thm3060_get_reg8(thm3060, THM3060_REG_RSTAT, &irq);


		if (irq & 0x01) {
			// data frame reception	correct
			break;
		}

		if (irq & 0x02) {
		// CRC Error
			return STATUS_CRC_WRONG;
		}

		if (irq & 0x04) {			// Timer interrupt - nothing received in 25ms
			return STATUS_TIMEOUT;
		}

		if (irq & 0x08) {
			// Frame data overflow
			return STATUS_ERROR;
		}

		if (irq & 0x10) {
			// frame format error
			return STATUS_ERROR;
		}

		if (irq & 0x20) {
			// Parity Error
			return STATUS_ERROR;
		}

		if (irq & 0x40) {
			collision = true;
		}	break;

		if (irq & 0x80) {
			// Interrupt Detected
			// What caused the interrupt?
			// break;
		}


	}
	// 35.7ms and nothing happend. Communication with the MFRC522 might be down.
	if ( (HAL_GetTick() - begin) >= 36) {
		return STATUS_TIMEOUT;
	}

	thm3060_and_reg8(thm3060,THM3060_REG_SCON, ~0x02); // stop

	uint8_t _validBits = 0;

	// If the caller wants data back, get it from the MFRC522.
	if (backData && backLen) {
		uint8_t fifo_data_len;
		thm3060_get_reg8(thm3060, THM3060_REG_RSCL, &fifo_data_len);// Number of bytes in the FIFO

		if (fifo_data_len > *backLen) {
			return STATUS_NO_ROOM;
		}
		*backLen = fifo_data_len;							// Number of bytes returned
		thm3060_recv(thm3060, THM3060_REG_DATA, backData, fifo_data_len);

		thm3060_get_reg8(thm3060, THM3060_REG_BPOS, &_validBits);
		_validBits &= 0x07;// RxLastBits[2:0] indicates the number of valid bits in the last received byte. If this value is 000b, the whole uint8_t is valid.
		if (validBits) {
			*validBits = _validBits;
		}
	}

	// Tell about collisions
	if (collision) {		// CollErr
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

rc52x_result_t  THM3060_TransceiveData(thm3060_t *thm3060,uint8_t *sendData,
		uint8_t sendLen,
		uint8_t *backData,
		uint8_t *backLen,
		uint8_t *validBits,
		uint8_t rxAlign,
		uint8_t * collpos,
		bool sendCRC ,
		bool recvCRC
		) {

	return  THM3060_CommunicateWithPICC(thm3060, 0, 0,
			sendData, sendLen, backData, backLen, validBits, rxAlign,  sendCRC, recvCRC);
} // End RC52X_TransceiveData()


void THM3060_Init(thm3060_t *thm3060) {
	thm3060->TransceiveData= THM3060_TransceiveData;
	// 12.6.1	Set the protocol by the PSEL register (TYPE-A)
	thm3060_set_reg8(thm3060,THM3060_REG_PSEL, 0b00010000); // Type A, 106 kbps
	// 12.6.2 	Set the CRCSEL register (generate CRC automatically)
	thm3060_set_reg8(thm3060,THM3060_REG_CRCSEL, 0b00000001); // No CRC, timeout enabled

	// 12.6.3	Set the TMR register
	//thm3060_set_reg8(thm3060,THM3060_REG_TMRH, 0x00);
	thm3060_set_reg8(thm3060,THM3060_REG_TMRH, 0xFF);
	thm3060_set_reg8(thm3060,THM3060_REG_TMRL, 0xFF);

	// 12.6.4	Open the RF carrier (SCON)
	THM3060_AntennaOn(thm3060);
}
