/*
 * MFRC522.cpp - Library to use ARDUINO RFID MODULE KIT 13.56 MHZ WITH TAGS SPI W AND R BY COOQROBOT.
 * NOTE: Please also check the comments in MFRC522.h - they provide useful hints and background information.
 * Released into the public domain.
 */

#include "MFRC522.h"
#include "rc52x.h"

// Temporary helper
// I don't like this style of function as it offers no
// possibility for error handling. Adding it to speed up portong
uint8_t PCD_ReadRegister(rc52x_t *rc52x, uint8_t reg) {
	uint8_t regval;
	int result = rc52x_get_reg8(rc52x, reg, &regval);
	(void) result;  // We do not handle the error!
	return regval;
}

/**
 * Use the CRC coprocessor in the MFRC522 to calculate a CRC_A.
 * 
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t PCD_CalculateCRC(rc52x_t *rc52x, uint8_t *data, ///< In: Pointer to the data to transfer to the FIFO for CRC calculation.
		uint8_t length,	///< In: The number of uint8_ts to transfer.
		uint8_t *result	///< Out: Pointer to result buffer. Result is written to result[0..1], low uint8_t first.
		) {
	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, MFRC_CMD_Idle);// Stop any active command.
	rc52x_set_reg8(rc52x, MFRC_REG_DivIrqReg, 0x04);// Clear the CRCIRq interrupt request bit
	rc52x_set_reg8(rc52x, MFRC_REG_FIFOLevelReg, 0x80);	// FlushBuffer = 1, FIFO initialization
	rc52x_set_reg8(rc52x, MFRC_REG_FIFODataReg, length, data);// Write data to the FIFO
	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, MFRC_CMD_CalcCRC);// Start the calculation

	// Wait for the CRC calculation to complete. Each iteration of the while-loop takes 17.73μs.
	// TODO check/modify for other architectures than Arduino Uno 16bit

	// Wait for the CRC calculation to complete. Each iteration of the while-loop takes 17.73us.
	for (uint16_t i = 5000; i > 0; i--) {
		// DivIrqReg[7..0] bits are: Set2 reserved reserved MfinActIRq reserved CRCIRq reserved reserved
		uint8_t n = PCD_ReadRegister(rc52x, MFRC_REG_DivIrqReg);
		if (n & 0x04) {						// CRCIRq bit set - calculation done
			rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, MFRC_CMD_Idle);// Stop calculating CRC for new content in the FIFO.
			// Transfer the result from the registers to the result buffer
			result[0] = PCD_ReadRegister(rc52x, MFRC_REG_CRCResultReg_Lo);
			result[1] = PCD_ReadRegister(rc52x, MFRC_REG_CRCResultReg_Hi);
			return STATUS_OK;
		}
	}
	// 89ms passed and nothing happend. Communication with the MFRC522 might be down.
	return STATUS_TIMEOUT;
} // End PCD_CalculateCRC()

/////////////////////////////////////////////////////////////////////////////////////
// Functions for manipulating the MFRC522
/////////////////////////////////////////////////////////////////////////////////////

/**
 * Initializes the MFRC522 chip.
 */
void PCD_Init(rc52x_t *rc52x) {

	PCD_Reset(rc52x);

	// Reset baud rates
	rc52x_set_reg8(rc52x, MFRC_REG_TxModeReg, 0x00);
	rc52x_set_reg8(rc52x, MFRC_REG_RxModeReg, 0x00);
	// Reset ModWidthReg
	rc52x_set_reg8(rc52x, MFRC_REG_ModWidthReg, 0x26);

	// When communicating with a PICC we need a timeout if something goes wrong.
	// f_timer = 13.56 MHz / (2*TPreScaler+1) where TPreScaler = [TPrescaler_Hi:TPrescaler_Lo].
	// TPrescaler_Hi are the four low bits in TModeReg. TPrescaler_Lo is TPrescalerReg.
	rc52x_set_reg8(rc52x, MFRC_REG_TModeReg, 0x80);	// TAuto=1; timer starts automatically at the end of the transmission in all communication modes at all speeds
	rc52x_set_reg8(rc52x, MFRC_REG_TPrescalerReg, 0xA9);// TPreScaler = TModeReg[3..0]:TPrescalerReg, ie 0x0A9 = 169 => f_timer=40kHz, ie a timer period of 25μs.
	rc52x_set_reg8(rc52x, MFRC_REG_TReloadReg_Hi, 0x03);// Reload timer with 0x3E8 = 1000, ie 25ms before timeout.
	rc52x_set_reg8(rc52x, MFRC_REG_TReloadReg_Lo, 0xE8);

	rc52x_set_reg8(rc52x, MFRC_REG_TxASKReg, 0x40);	// Default 0x00. Force a 100 % ASK modulation independent of the ModGsPReg register setting
	rc52x_set_reg8(rc52x, MFRC_REG_ModeReg, 0x3D);// Default 0x3F. Set the preset value for the CRC coprocessor for the CalcCRC command to 0x6363 (ISO 14443-3 part 6.2.4)
	PCD_AntennaOn(rc52x);// Enable the antenna driver pins TX1 and TX2 (they were disabled by the reset)
} // End PCD_Init()

/**
 * Performs a soft reset on the MFRC522 chip and waits for it to be ready again.
 */
void PCD_Reset(rc52x_t *rc52x) {
	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, MFRC_CMD_SoftReset); // Issue the SoftReset command.
	// The datasheet does not mention how long the SoftRest command takes to complete.
	// But the MFRC522 might have been in soft power-down mode (triggered by bit 4 of CommandReg) 
	// Section 8.8.2 in the datasheet says the oscillator start-up time is the start up time of the crystal + 37,74μs. Let us be generous: 50ms.
	uint8_t count = 0;
	do {
		// Wait for the PowerDown bit in CommandReg to be cleared (max 3x50ms)
		delay(50);
	} while ((PCD_ReadRegister(rc52x, MFRC_REG_CommandReg) & (1 << 4))
			&& (++count) < 3);
} // End PCD_Reset()

/**
 * Turns the antenna on by enabling pins TX1 and TX2.
 * After a reset these pins are disabled.
 */
void PCD_AntennaOn(rc52x_t *rc52x) {
	uint8_t value = PCD_ReadRegister(rc52x, MFRC_REG_TxControlReg);
	if ((value & 0x03) != 0x03) {
		rc52x_set_reg8(rc52x, MFRC_REG_TxControlReg, value | 0x03);
	}
} // End PCD_AntennaOn()

/**
 * Turns the antenna off by disabling pins TX1 and TX2.
 */
void PCD_AntennaOff(rc52x_t *rc52x) {
	PCD_ClearRegisterBitMask(MFRC_REG_TxControlReg, 0x03);
} // End PCD_AntennaOff()

/**
 * Get the current MFRC522 Receiver Gain (RxGain[2:0]) value.
 * See 9.3.3.6 / table 98 in http://www.nxp.com/documents/data_sheet/MFRC522.pdf
 * NOTE: Return value scrubbed with (0x07<<4)=01110000b as RCFfgReg may use reserved bits.
 * 
 * @return Value of the RxGain, scrubbed to the 3 bits used.
 */
uint8_t PCD_GetAntennaGain(rc52x_t *rc52x) {
	return PCD_ReadRegister(rc52x, MFRC_REG_RFCfgReg) & (0x07 << 4);
} // End PCD_GetAntennaGain()

/**
 * Set the MFRC522 Receiver Gain (RxGain) to value specified by given mask.
 * See 9.3.3.6 / table 98 in http://www.nxp.com/documents/data_sheet/MFRC522.pdf
 * NOTE: Given mask is scrubbed with (0x07<<4)=01110000b as RCFfgReg may use reserved bits.
 */
void PCD_SetAntennaGain(rc52x_t *rc52x, uint8_t mask) {
	if (PCD_GetAntennaGain(rc52x) != mask) {		// only bother if there is a change
		PCD_ClearRegisterBitMask(rc52x, MFRC_REG_RFCfgReg, (0x07 << 4));// clear needed to allow 000 pattern
		PCD_SetRegisterBitMask(rc52x, MFRC_REG_RFCfgReg, mask & (0x07 << 4));// only set RxGain[2:0] bits
	}
} // End PCD_SetAntennaGain()

/**
 * Performs a self-test of the MFRC522
 * See 16.1.1 in http://www.nxp.com/documents/data_sheet/MFRC522.pdf
 * 
 * @return Whether or not the test passed. Or false if no firmware reference is available.
 */
bool PCD_PerformSelfTest(rc52x_t *rc52x) {
	// This follows directly the steps outlined in 16.1.1
	// 1. Perform a soft reset.
	PCD_Reset(rc52x);

	// 2. Clear the internal buffer by writing 25 uint8_ts of 00h
	uint8_t ZEROES[25] = { 0x00 };
	rc52x_set_reg8(rc52x, MFRC_REG_FIFOLevelReg, 0x80);	// flush the FIFO buffer
	rc52x_set_reg8(rc52x, MFRC_REG_FIFODataReg, 25, ZEROES);// write 25 uint8_ts of 00h to FIFO
	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, MFRC_CMD_Configure);// transfer to internal buffer

	// 3. Enable self-test
	rc52x_set_reg8(rc52x, MFRC_REG_AutoTestReg, 0x09);

	// 4. Write 00h to FIFO buffer
	rc52x_set_reg8(rc52x, MFRC_REG_FIFODataReg, 0x00);

	// 5. Start self-test by issuing the CalcCRC command
	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, MFRC_CMD_CalcCRC);

	// 6. Wait for self-test to complete
	uint8_t n;
	for (uint8_t i = 0; i < 0xFF; i++) {
		// The datasheet does not specify exact completion condition except
		// that FIFO buffer should contain 64 uint8_ts.
		// While selftest is initiated by CalcCRC command
		// it behaves differently from normal CRC computation,
		// so one can't reliably use DivIrqReg to check for completion.
		// It is reported that some devices does not trigger CRCIRq flag
		// during selftest.
		n = PCD_ReadRegister(rc52x, MFRC_REG_FIFOLevelReg);
		if (n >= 64) {
			break;
		}
	}
	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, MFRC_CMD_Idle);// Stop calculating CRC for new content in the FIFO.

	// 7. Read out resulting 64 uint8_ts from the FIFO buffer.
	uint8_t result[64];
	 mfrc522_recv(rc52x, MFRC_REG_FIFODataReg, result, 64);

	// Auto self-test done
	// Reset AutoTestReg register to be 0 again. Required for normal operation.
	rc52x_set_reg8(rc52x, MFRC_REG_AutoTestReg, 0x00);

	// Determine firmware version (see section 9.3.4.8 in spec)
	uint8_t version = PCD_ReadRegister(rc52x,MFRC_REG_VersionReg);

	// Pick the appropriate reference values
	const uint8_t *reference;
	switch (version) {
	case 0x88:	// Fudan Semiconductor FM17522 clone
		reference = FM17522_firmware_reference;
		break;
	case 0x90:	// Version 0.0
		reference = MFRC522_firmware_referenceV0_0;
		break;
	case 0x91:	// Version 1.0
		reference = MFRC522_firmware_referenceV1_0;
		break;
	case 0x92:	// Version 2.0
		reference = MFRC522_firmware_referenceV2_0;
		break;
	default:	// Unknown version
		return false; // abort test
	}

	/*
	 *
	 *  Some Atmel specific stuff
	// Verify that the results match up to our expectations
	for (uint8_t i = 0; i < 64; i++) {
		if (result[i] != pgm_read_uint8_t(&(reference[i]))) {
			return false;
		}
	}
	*/

	// Test passed; all is good.
	return true;
} // End PCD_PerformSelfTest()

/////////////////////////////////////////////////////////////////////////////////////
// Power control
/////////////////////////////////////////////////////////////////////////////////////

//IMPORTANT NOTE!!!!
//Calling any other function that uses CommandReg will disable soft power down mode !!!
//For more details about power control, refer to the datasheet - page 33 (8.6)

void PCD_SoftPowerDown(rc52x_t *rc52x) { //Note : Only soft power down mode is available throught software
	uint8_t val = PCD_ReadRegister(rc52x, MFRC_REG_CommandReg); // Read state of the command register
	val |= (1 << 4); // set PowerDown bit ( bit 4 ) to 1
	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, val); //write new value to the command register
}

void PCD_SoftPowerUp(rc52x_t *rc52x) {
	uint8_t val = PCD_ReadRegister(rc52x, MFRC_REG_CommandReg); // Read state of the command register
	val &= ~(1 << 4); // set PowerDown bit ( bit 4 ) to 0
	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, val); //write new value to the command register
	// wait until PowerDown bit is cleared (this indicates end of wake up procedure) 
	const uint32_t timeout = (uint32_t) millis() + 500;	// create timer for timeout (just in case)

	while (millis() <= timeout) { // set timeout to 500 ms
		val = PCD_ReadRegister(rc52x, MFRC_REG_CommandReg); // Read state of the command register
		if (!(val & (1 << 4))) { // if powerdown bit is 0
			break; // wake up procedure is finished
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Functions for communicating with PICCs
/////////////////////////////////////////////////////////////////////////////////////

/**
 * Executes the Transceive command.
 * CRC validation can only be done if backData and backLen are specified.
 * 
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t PCD_TransceiveData(rc52x_t *rc52x, uint8_t *sendData, ///< Pointer to the data to transfer to the FIFO.
		uint8_t sendLen,		///< Number of uint8_ts to transfer to the FIFO.
		uint8_t *backData,///< nullptr or pointer to buffer if data should be read back after executing the command.
		uint8_t *backLen,///< In: Max number of uint8_ts to write to *backData. Out: The number of uint8_ts returned.
		uint8_t *validBits,	///< In/Out: The number of valid bits in the last uint8_t. 0 for 8 valid bits. Default nullptr.
		uint8_t rxAlign,///< In: Defines the bit position in backData[0] for the first bit received. Default 0.
		bool checkCRC///< In: True => The last two uint8_ts of the response is assumed to be a CRC_A that must be validated.
		) {
	uint8_t waitIRq = 0x30;		// RxIRq and IdleIRq
	return PCD_CommunicateWithPICC(rc52x, MFRC_CMD_Transceive, waitIRq, sendData,
			sendLen, backData, backLen, validBits, rxAlign, checkCRC);
} // End PCD_TransceiveData()

/**
 * Transfers data to the MFRC522 FIFO, executes a command, waits for completion and transfers data back from the FIFO.
 * CRC validation can only be done if backData and backLen are specified.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t PCD_CommunicateWithPICC(rc52x_t *rc52x, uint8_t command,	///< The command to execute. One of the PCD_Command enums.
		uint8_t waitIRq,///< The bits in the ComIrqReg register that signals successful completion of the command.
		uint8_t *sendData,	///< Pointer to the data to transfer to the FIFO.
		uint8_t sendLen,		///< Number of uint8_ts to transfer to the FIFO.
		uint8_t *backData,///< nullptr or pointer to buffer if data should be read back after executing the command.
		uint8_t *backLen,///< In: Max number of uint8_ts to write to *backData. Out: The number of uint8_ts returned.
		uint8_t *validBits,	///< In/Out: The number of valid bits in the last uint8_t. 0 for 8 valid bits.
		uint8_t rxAlign,///< In: Defines the bit position in backData[0] for the first bit received. Default 0.
		bool checkCRC///< In: True => The last two uint8_ts of the response is assumed to be a CRC_A that must be validated.
		) {
	// Prepare values for BitFramingReg
	uint8_t txLastBits = validBits ? *validBits : 0;
	uint8_t bitFraming = (rxAlign << 4) + txLastBits;// RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]

	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, MFRC_CMD_Idle);// Stop any active command.
	rc52x_set_reg8(rc52x, MFRC_REG_ComIrqReg, 0x7F);// Clear all seven interrupt request bits
	rc52x_set_reg8(rc52x, MFRC_REG_FIFOLevelReg, 0x80);	// FlushBuffer = 1, FIFO initialization
	mfrc522_send(rc52x, MFRC_REG_FIFODataReg, sendData, sendLen);	// Write sendData to the FIFO
	rc52x_set_reg8(rc52x, MFRC_REG_BitFramingReg, bitFraming);// Bit adjustments
	rc52x_set_reg8(rc52x, MFRC_REG_CommandReg, command);// Execute the command
	if (command == MFRC_CMD_Transceive) {
		PCD_SetRegisterBitMask(rc52x, MFRC_REG_BitFramingReg, 0x80);// StartSend=1, transmission of data starts
	}

	// Wait for the command to complete.
	// In PCD_Init() we set the TAuto flag in TModeReg. This means the timer automatically starts when the PCD stops transmitting.
	// Each iteration of the do-while-loop takes 17.86μs.
	// TODO check/modify for other architectures than Arduino Uno 16bit
	uint16_t i;
	for (i = 2000; i > 0; i--) {
		uint8_t n = PCD_ReadRegister(rc52x, MFRC_REG_ComIrqReg);// ComIrqReg[7..0] bits are: Set1 TxIRq RxIRq IdleIRq HiAlertIRq LoAlertIRq ErrIRq TimerIRq
		if (n & waitIRq) {// One of the interrupts that signal success has been set.
			break;
		}
		if (n & 0x01) {			// Timer interrupt - nothing received in 25ms
			return STATUS_TIMEOUT;
		}
	}
	// 35.7ms and nothing happend. Communication with the MFRC522 might be down.
	if (i == 0) {
		return STATUS_TIMEOUT;
	}

	// Stop now if any errors except collisions were detected.
	uint8_t errorRegValue = PCD_ReadRegister(rc52x, MFRC_REG_ErrorReg); // ErrorReg[7..0] bits are: WrErr TempErr reserved BufferOvfl CollErr CRCErr ParityErr ProtocolErr
	if (errorRegValue & 0x13) {	 // BufferOvfl ParityErr ProtocolErr
		return STATUS_ERROR;
	}

	uint8_t _validBits = 0;

	// If the caller wants data back, get it from the MFRC522.
	if (backData && backLen) {
		uint8_t n = PCD_ReadRegister(rc52x, MFRC_REG_FIFOLevelReg);	// Number of uint8_ts in the FIFO
		if (n > *backLen) {
			return STATUS_NO_ROOM;
		}
		*backLen = n;							// Number of uint8_ts returned
		mfrc522_recv(rc52x, MFRC_REG_FIFODataReg, backData, n);
		_validBits = PCD_ReadRegister(rc52x, MFRC_REG_ControlReg) & 0x07;// RxLastBits[2:0] indicates the number of valid bits in the last received uint8_t. If this value is 000b, the whole uint8_t is valid.
		if (validBits) {
			*validBits = _validBits;
		}
	}

	// Tell about collisions
	if (errorRegValue & 0x08) {		// CollErr
		return STATUS_COLLISION;
	}

	// Perform CRC_A validation if requested.
	if (backData && backLen && checkCRC) {
		// In this case a MIFARE Classic NAK is not OK.
		if (*backLen == 1 && _validBits == 4) {
			return STATUS_MIFARE_NACK;
		}
		// We need at least the CRC_A value and all 8 bits of the last uint8_t must be received.
		if (*backLen < 2 || _validBits != 0) {
			return STATUS_CRC_WRONG;
		}
		// Verify CRC_A - do our own calculation and store the control in controlBuffer.
		uint8_t controlBuffer[2];
		rc52x_result_t status = PCD_CalculateCRC(rc52x, &backData[0], *backLen - 2,
				&controlBuffer[0]);
		if (status != STATUS_OK) {
			return status;
		}
		if ((backData[*backLen - 2] != controlBuffer[0])
				|| (backData[*backLen - 1] != controlBuffer[1])) {
			return STATUS_CRC_WRONG;
		}
	}

	return STATUS_OK;
} // End PCD_CommunicateWithPICC()

/**
 * Transmits a REQuest command, Type A. Invites PICCs in state IDLE to go to READY and prepare for anticollision or selection. 7 bit frame.
 * Beware: When two PICCs are in the field at the same time I often get STATUS_TIMEOUT - probably due do bad antenna design.
 * 
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t PICC_RequestA(rc52x_t *rc52x, uint8_t *bufferATQA, ///< The buffer to store the ATQA (Answer to request) in
		uint8_t *bufferSize	///< Buffer size, at least two uint8_ts. Also number of uint8_ts returned if STATUS_OK.
		) {
	return PICC_REQA_or_WUPA(rc52x, PICC_CMD_REQA, bufferATQA, bufferSize);
} // End PICC_RequestA()

/**
 * Transmits a Wake-UP command, Type A. Invites PICCs in state IDLE and HALT to go to READY(*) and prepare for anticollision or selection. 7 bit frame.
 * Beware: When two PICCs are in the field at the same time I often get STATUS_TIMEOUT - probably due do bad antenna design.
 * 
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t PICC_WakeupA(rc52x_t *rc52x, uint8_t *bufferATQA, ///< The buffer to store the ATQA (Answer to request) in
		uint8_t *bufferSize	///< Buffer size, at least two uint8_ts. Also number of uint8_ts returned if STATUS_OK.
		) {
	return PICC_REQA_or_WUPA(rc52x, PICC_CMD_WUPA, bufferATQA, bufferSize);
} // End PICC_WakeupA()

/**
 * Transmits REQA or WUPA commands.
 * Beware: When two PICCs are in the field at the same time I often get STATUS_TIMEOUT - probably due do bad antenna design.
 * 
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t PICC_REQA_or_WUPA(rc52x_t *rc52x, uint8_t command, ///< The command to send - PICC_CMD_REQA or PICC_CMD_WUPA
		uint8_t *bufferATQA, ///< The buffer to store the ATQA (Answer to request) in
		uint8_t *bufferSize	///< Buffer size, at least two uint8_ts. Also number of uint8_ts returned if STATUS_OK.
		) {
	uint8_t validBits;
	rc52x_result_t status;

	if (bufferATQA == NULL || *bufferSize < 2) {	// The ATQA response is 2 uint8_ts long.
		return STATUS_NO_ROOM;
	}
	PCD_ClearRegisterBitMask(MFRC_REG_CollReg, 0x80);// ValuesAfterColl=1 => Bits received after collision are cleared.
	validBits = 7;// For REQA and WUPA we need the short frame format - transmit only 7 bits of the last (and only) uint8_t. TxLastBits = BitFramingReg[2..0]
	status = PCD_TransceiveData(rc52x, &command, 1, bufferATQA, bufferSize,
			&validBits);
	if (status != STATUS_OK) {
		return status;
	}
	if (*bufferSize != 2 || validBits != 0) {	// ATQA must be exactly 16 bits.
		return STATUS_ERROR;
	}
	return STATUS_OK;
} // End PICC_REQA_or_WUPA()

/**
 * Transmits SELECT/ANTICOLLISION commands to select a single PICC.
 * Before calling this function the PICCs must be placed in the READY(*) state by calling PICC_RequestA() or PICC_WakeupA().
 * On success:
 * 		- The chosen PICC is in state ACTIVE(*) and all other PICCs have returned to state IDLE/HALT. (Figure 7 of the ISO/IEC 14443-3 draft.)
 * 		- The UID size and value of the chosen PICC is returned in *uid along with the SAK.
 * 
 * A PICC UID consists of 4, 7 or 10 uint8_ts.
 * Only 4 uint8_ts can be specified in a SELECT command, so for the longer UIDs two or three iterations are used:
 * 		UID size	Number of UID uint8_ts		Cascade levels		Example of PICC
 * 		========	===================		==============		===============
 * 		single				 4						1				MIFARE Classic
 * 		double				 7						2				MIFARE Ultralight
 * 		triple				10						3				Not currently in use?
 * 
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t PICC_Select(rc52x_t *rc52x, Uid *uid, ///< Pointer to Uid struct. Normally output, but can also be used to supply a known UID.
		uint8_t validBits ///< The number of known UID bits supplied in *uid. Normally 0. If set you must also supply uid->size.
		) {
	bool uidComplete;
	bool selectDone;
	bool useCascadeTag;
	uint8_t cascadeLevel = 1;
	rc52x_result_t result;
	uint8_t count;
	uint8_t checkBit;
	uint8_t index;
	uint8_t uidIndex; // The first index in uid->uiduint8_t[] that is used in the current Cascade Level.
	int8_t currentLevelKnownBits; // The number of known UID bits in the current Cascade Level.
	uint8_t buffer[9]; // The SELECT/ANTICOLLISION commands uses a 7 uint8_t standard frame + 2 uint8_ts CRC_A
	uint8_t bufferUsed;	// The number of uint8_ts used in the buffer, ie the number of uint8_ts to transfer to the FIFO.
	uint8_t rxAlign;// Used in BitFramingReg. Defines the bit position for the first bit received.
	uint8_t txLastBits;	// Used in BitFramingReg. The number of valid bits in the last transmitted uint8_t.
	uint8_t *responseBuffer;
	uint8_t responseLength;

	// Description of buffer structure:
	//		uint8_t 0: SEL 				Indicates the Cascade Level: PICC_CMD_SEL_CL1, PICC_CMD_SEL_CL2 or PICC_CMD_SEL_CL3
	//		uint8_t 1: NVB					Number of Valid Bits (in complete command, not just the UID): High nibble: complete uint8_ts, Low nibble: Extra bits. 
	//		uint8_t 2: UID-data or CT		See explanation below. CT means Cascade Tag.
	//		uint8_t 3: UID-data
	//		uint8_t 4: UID-data
	//		uint8_t 5: UID-data
	//		uint8_t 6: BCC					Block Check Character - XOR of uint8_ts 2-5
	//		uint8_t 7: CRC_A
	//		uint8_t 8: CRC_A
	// The BCC and CRC_A are only transmitted if we know all the UID bits of the current Cascade Level.
	//
	// Description of uint8_ts 2-5: (Section 6.5.4 of the ISO/IEC 14443-3 draft: UID contents and cascade levels)
	//		UID size	Cascade level	uint8_t2	uint8_t3	uint8_t4	uint8_t5
	//		========	=============	=====	=====	=====	=====
	//		 4 uint8_ts		1			uid0	uid1	uid2	uid3
	//		 7 uint8_ts		1			CT		uid0	uid1	uid2
	//						2			uid3	uid4	uid5	uid6
	//		10 uint8_ts		1			CT		uid0	uid1	uid2
	//						2			CT		uid3	uid4	uid5
	//						3			uid6	uid7	uid8	uid9

	// Sanity checks
	if (validBits > 80) {
		return STATUS_INVALID;
	}

	// Prepare MFRC522
	PCD_ClearRegisterBitMask(CollReg, 0x80);// ValuesAfterColl=1 => Bits received after collision are cleared.

	// Repeat Cascade Level loop until we have a complete UID.
	uidComplete = false;
	while (!uidComplete) {
		// Set the Cascade Level in the SEL uint8_t, find out if we need to use the Cascade Tag in uint8_t 2.
		switch (cascadeLevel) {
		case 1:
			buffer[0] = PICC_CMD_SEL_CL1;
			uidIndex = 0;
			useCascadeTag = validBits && uid->size > 4;	// When we know that the UID has more than 4 uint8_ts
			break;

		case 2:
			buffer[0] = PICC_CMD_SEL_CL2;
			uidIndex = 3;
			useCascadeTag = validBits && uid->size > 7;	// When we know that the UID has more than 7 uint8_ts
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
		// Copy the known bits from uid->uiduint8_t[] to buffer[]
		index = 2; // destination index in buffer[]
		if (useCascadeTag) {
			buffer[index++] = PICC_CMD_CT;
		}
		uint8_t uint8_tsToCopy = currentLevelKnownBits / 8
				+ (currentLevelKnownBits % 8 ? 1 : 0); // The number of uint8_ts needed to represent the known bits for this level.
		if (uint8_tsToCopy) {
			uint8_t maxuint8_ts = useCascadeTag ? 3 : 4; // Max 4 uint8_ts in each Cascade Level. Only 3 left if we use the Cascade Tag
			if (uint8_tsToCopy > maxuint8_ts) {
				uint8_tsToCopy = maxuint8_ts;
			}
			for (count = 0; count < uint8_tsToCopy; count++) {
				buffer[index++] = uid->uiduint8_t[uidIndex + count];
			}
		}
		// Now that the data has been copied we need to include the 8 bits in CT in currentLevelKnownBits
		if (useCascadeTag) {
			currentLevelKnownBits += 8;
		}

		// Repeat anti collision loop until we can transmit all UID bits + BCC and receive a SAK - max 32 iterations.
		selectDone = false;
		while (!selectDone) {
			// Find out how many bits and uint8_ts to send and receive.
			if (currentLevelKnownBits >= 32) { // All UID bits in this Cascade Level are known. This is a SELECT.
				//Serial.print(F("SELECT: currentLevelKnownBits=")); Serial.println(currentLevelKnownBits, DEC);
				buffer[1] = 0x70; // NVB - Number of Valid Bits: Seven whole uint8_ts
				// Calculate BCC - Block Check Character
				buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];
				// Calculate CRC_A
				result = PCD_CalculateCRC(buffer, 7, &buffer[7]);
				if (result != STATUS_OK) {
					return result;
				}
				txLastBits = 0; // 0 => All 8 bits are valid.
				bufferUsed = 9;
				// Store response in the last 3 uint8_ts of buffer (BCC and CRC_A - not needed after tx)
				responseBuffer = &buffer[6];
				responseLength = 3;
			} else { // This is an ANTICOLLISION.
					 //Serial.print(F("ANTICOLLISION: currentLevelKnownBits=")); Serial.println(currentLevelKnownBits, DEC);
				txLastBits = currentLevelKnownBits % 8;
				count = currentLevelKnownBits / 8;// Number of whole uint8_ts in the UID part.
				index = 2 + count;// Number of whole uint8_ts: SEL + NVB + UIDs
				buffer[1] = (index << 4) + txLastBits;// NVB - Number of Valid Bits
				bufferUsed = index + (txLastBits ? 1 : 0);
				// Store response in the unused part of buffer
				responseBuffer = &buffer[index];
				responseLength = sizeof(buffer) - index;
			}

			// Set bit adjustments
			rxAlign = txLastBits;// Having a separate variable is overkill. But it makes the next line easier to read.
			rc52x_set_reg8(rc52x, MFRC_REG_BitFramingReg,
					(rxAlign << 4) + txLastBits);// RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]

			// Transmit the buffer and receive the response.
			result = PCD_TransceiveData(rc52x, buffer, bufferUsed,
					responseBuffer, &responseLength, &txLastBits, rxAlign);
			if (result == STATUS_COLLISION) { // More than one PICC in the field => collision.
				uint8_t valueOfCollReg = PCD_ReadRegister(CollReg); // CollReg[7..0] bits are: ValuesAfterColl reserved CollPosNotValid CollPos[4:0]
				if (valueOfCollReg & 0x20) { // CollPosNotValid
					return STATUS_COLLISION; // Without a valid collision position we cannot continue
				}
				uint8_t collisionPos = valueOfCollReg & 0x1F; // Values 0-31, 0 means bit 32.
				if (collisionPos == 0) {
					collisionPos = 32;
				}
				if (collisionPos <= currentLevelKnownBits) { // No progress - should not happen 
					return STATUS_INTERNAL_ERROR;
				}
				// Choose the PICC with the bit set.
				currentLevelKnownBits = collisionPos;
				count = currentLevelKnownBits % 8; // The bit to modify
				checkBit = (currentLevelKnownBits - 1) % 8;
				index = 1 + (currentLevelKnownBits / 8) + (count ? 1 : 0); // First uint8_t is index 0.
				buffer[index] |= (1 << checkBit);
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
		} // End of while (!selectDone)

		// We do not check the CBB - it was constructed by us above.

		// Copy the found UID uint8_ts from buffer[] to uid->uiduint8_t[]
		index = (buffer[2] == PICC_CMD_CT) ? 3 : 2; // source index in buffer[]
		uint8_tsToCopy = (buffer[2] == PICC_CMD_CT) ? 3 : 4;
		for (count = 0; count < uint8_tsToCopy; count++) {
			uid->uiduint8_t[uidIndex + count] = buffer[index++];
		}

		// Check response SAK (Select Acknowledge)
		if (responseLength != 3 || txLastBits != 0) { // SAK must be exactly 24 bits (1 uint8_t + CRC_A).
			return STATUS_ERROR;
		}
		// Verify CRC_A - do our own calculation and store the control in buffer[2..3] - those uint8_ts are not needed anymore.
		result = PCD_CalculateCRC(rc52x, responseBuffer, 1, &buffer[2]);
		if (result != STATUS_OK) {
			return result;
		}
		if ((buffer[2] != responseBuffer[1])
				|| (buffer[3] != responseBuffer[2])) {
			return STATUS_CRC_WRONG;
		}
		if (responseBuffer[0] & 0x04) { // Cascade bit set - UID not complete yes
			cascadeLevel++;
		} else {
			uidComplete = true;
			uid->sak = responseBuffer[0];
		}
	} // End of while (!uidComplete)

	// Set correct uid->size
	uid->size = 3 * cascadeLevel + 1;

	return STATUS_OK;
} // End PICC_Select()

/**
 * Instructs a PICC in state ACTIVE(*) to go to state HALT.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t PICC_HaltA(rc52x_t *rc52x) {
	rc52x_result_t result;
	uint8_t buffer[4];

	// Build command buffer
	buffer[0] = PICC_CMD_HLTA;
	buffer[1] = 0;
	// Calculate CRC_A
	result = PCD_CalculateCRC(rc52x, buffer, 2, &buffer[2]);
	if (result != STATUS_OK) {
		return result;
	}

	// Send the command.
	// The standard says:
	//		If the PICC responds with any modulation during a period of 1 ms after the end of the frame containing the
	//		HLTA command, this response shall be interpreted as 'not acknowledge'.
	// We interpret that this way: Only STATUS_TIMEOUT is a success.
	result = PCD_TransceiveData(rc52x, buffer, sizeof(buffer), nullptr, 0);
	if (result == STATUS_TIMEOUT) {
		return STATUS_OK;
	}
	if (result == STATUS_OK) { // That is ironically NOT ok in this case ;-)
		return STATUS_ERROR;
	}
	return result;
} // End PICC_HaltA()

/////////////////////////////////////////////////////////////////////////////////////
// Functions for communicating with MIFARE PICCs
/////////////////////////////////////////////////////////////////////////////////////

/**
 * Executes the MFRC522 MFAuthent command.
 * This command manages MIFARE authentication to enable a secure communication to any MIFARE Mini, MIFARE 1K and MIFARE 4K card.
 * The authentication is described in the MFRC522 datasheet section 10.3.1.9 and http://www.nxp.com/documents/data_sheet/MF1S503x.pdf section 10.1.
 * For use with MIFARE Classic PICCs.
 * The PICC must be selected - ie in state ACTIVE(*) - before calling this function.
 * Remember to call PCD_StopCrypto1() after communicating with the authenticated PICC - otherwise no new communications can start.
 * 
 * All keys are set to FFFFFFFFFFFFh at chip delivery.
 * 
 * @return STATUS_OK on success, STATUS_??? otherwise. Probably STATUS_TIMEOUT if you supply the wrong key.
 */
rc52x_result_t PCD_Authenticate(rc52x_t *rc52x, uint8_t command, ///< PICC_CMD_MF_AUTH_KEY_A or PICC_CMD_MF_AUTH_KEY_B
		uint8_t blockAddr, ///< The block number. See numbering in the comments in the .h file.
		MIFARE_Key *key,	///< Pointer to the Crypteo1 key to use (6 uint8_ts)
		Uid *uid///< Pointer to Uid struct. The first 4 uint8_ts of the UID is used.
		) {
	uint8_t waitIRq = 0x10;		// IdleIRq

	// Build command buffer
	uint8_t sendData[12];
	sendData[0] = command;
	sendData[1] = blockAddr;
	for (uint8_t i = 0; i < MF_KEY_SIZE; i++) {	// 6 key uint8_ts
		sendData[2 + i] = key->keyuint8_t[i];
	}
	// Use the last uid uint8_ts as specified in http://cache.nxp.com/documents/application_note/AN10927.pdf
	// section 3.2.5 "MIFARE Classic Authentication".
	// The only missed case is the MF1Sxxxx shortcut activation,
	// but it requires cascade tag (CT) uint8_t, that is not part of uid.
	for (uint8_t i = 0; i < 4; i++) {		// The last 4 uint8_ts of the UID
		sendData[8 + i] = uid->uiduint8_t[i + uid->size - 4];
	}

	// Start the authentication.
	return PCD_CommunicateWithPICC(rc52x, PCD_MFAuthent, waitIRq, &sendData[0],
			sizeof(sendData));
} // End PCD_Authenticate()

/**
 * Used to exit the PCD from its authenticated state.
 * Remember to call this function after communicating with an authenticated PICC - otherwise no new communications can start.
 */
void PCD_StopCrypto1(rc52x_t *rc52x) {
	// Clear MFCrypto1On bit
	PCD_ClearRegisterBitMask(Status2Reg, 0x08); // Status2Reg[7..0] bits are: TempSensClear I2CForceHS reserved reserved MFCrypto1On ModemState[2:0]
} // End PCD_StopCrypto1()

/**
 * Authenticate with a NTAG216.
 * 
 * Only for NTAG216. First implemented by Gargantuanman.
 * 
 * @param[in]   passuint16_t   passuint16_t.
 * @param[in]   pACK       result success???.
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t PCD_NTAG216_AUTH(rc52x_t *rc52x, uint8_t *passuint16_t,
		uint8_t pACK[]) //Authenticate with 32bit passuint16_t
{
	// TODO: Fix cmdBuffer length and rxlength. They really should match.
	//       (Better still, rxlength should not even be necessary.)

	rc52x_result_t result;
	uint8_t cmdBuffer[18]; // We need room for 16 uint8_ts data and 2 uint8_ts CRC_A.

	cmdBuffer[0] = 0x1B; //Comando de autentificacion

	for (uint8_t i = 0; i < 4; i++)
		cmdBuffer[i + 1] = passuint16_t[i];

	result = PCD_CalculateCRC(rc52x, cmdBuffer, 5, &cmdBuffer[5]);

	if (result != STATUS_OK) {
		return result;
	}

	// Transceive the data, store the reply in cmdBuffer[]
	uint8_t waitIRq = 0x30;	// RxIRq and IdleIRq
//	uint8_t cmdBufferSize	= sizeof(cmdBuffer);
	uint8_t validBits = 0;
	uint8_t rxlength = 5;
	result = PCD_CommunicateWithPICC(rc52x, MFRC_CMD_Transceive, waitIRq,
			cmdBuffer, 7, cmdBuffer, &rxlength, &validBits);

	pACK[0] = cmdBuffer[0];
	pACK[1] = cmdBuffer[1];

	if (result != STATUS_OK) {
		return result;
	}

	return STATUS_OK;
} // End PCD_NTAG216_AUTH()

/////////////////////////////////////////////////////////////////////////////////////
// Support functions
/////////////////////////////////////////////////////////////////////////////////////

/**
 * Wrapper for MIFARE protocol communication.
 * Adds CRC_A, executes the Transceive command and checks that the response is MF_ACK or a timeout.
 * 
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t PCD_MIFARE_Transceive(rc52x_t *rc52x, uint8_t *sendData,	///< Pointer to the data to transfer to the FIFO. Do NOT include the CRC_A.
		uint8_t sendLen,		///< Number of uint8_ts in sendData.
		bool acceptTimeout	///< True => A timeout is also success
		) {
	rc52x_result_t result;
	uint8_t cmdBuffer[18]; // We need room for 16 uint8_ts data and 2 uint8_ts CRC_A.

	// Sanity check
	if (sendData == nullptr || sendLen > 16) {
		return STATUS_INVALID;
	}

	// Copy sendData[] to cmdBuffer[] and add CRC_A
	memcpy(cmdBuffer, sendData, sendLen);
	result = PCD_CalculateCRC(rc52x, cmdBuffer, sendLen, &cmdBuffer[sendLen]);
	if (result != STATUS_OK) {
		return result;
	}
	sendLen += 2;

	// Transceive the data, store the reply in cmdBuffer[]
	uint8_t waitIRq = 0x30;		// RxIRq and IdleIRq
	uint8_t cmdBufferSize = sizeof(cmdBuffer);
	uint8_t validBits = 0;
	result = PCD_CommunicateWithPICC(rc52x, MFRC_CMD_Transceive, waitIRq,
			cmdBuffer, sendLen, cmdBuffer, &cmdBufferSize, &validBits);
	if (acceptTimeout && result == STATUS_TIMEOUT) {
		return STATUS_OK;
	}
	if (result != STATUS_OK) {
		return result;
	}
	// The PICC must reply with a 4 bit ACK
	if (cmdBufferSize != 1 || validBits != 4) {
		return STATUS_ERROR;
	}
	if (cmdBuffer[0] != MF_ACK) {
		return STATUS_MIFARE_NACK;
	}
	return STATUS_OK;
} // End PCD_MIFARE_Transceive()

/////////////////////////////////////////////////////////////////////////////////////
// Convenience functions - does not add extra functionality
/////////////////////////////////////////////////////////////////////////////////////

/**
 * Returns true if a PICC responds to PICC_CMD_REQA.
 * Only "new" cards in state IDLE are invited. Sleeping cards in state HALT are ignored.
 * 
 * @return bool
 */
bool PICC_IsNewCardPresent(rc52x_t *rc52x) {
	uint8_t bufferATQA[2];
	uint8_t bufferSize = sizeof(bufferATQA);

	// Reset baud rates
	rc52x_set_reg8(rc52x, MFRC_REG_TxModeReg, 0x00);
	rc52x_set_reg8(rc52x, MFRC_REG_RxModeReg, 0x00);
	// Reset ModWidthReg
	rc52x_set_reg8(rc52x, MFRC_REG_ModWidthReg, 0x26);

	rc52x_result_t result = PICC_RequestA(rc52x, bufferATQA, &bufferSize);
	return (result == STATUS_OK || result == STATUS_COLLISION);
} // End PICC_IsNewCardPresent()

