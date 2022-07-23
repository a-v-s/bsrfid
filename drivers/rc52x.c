/*****************************************************************************
 * Based upon https://github.com/miguelbalboa/rfid
 *
 * C port by André van Schoubroeck <andre@blaatschaap.be>
 *
 *
 *****************************************************************************/

// For testing purposes, thsi will not be the final api
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "rc52x_transport.h"
#include "rc52x.h"

#include "iso14443a.h"





#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

//#include "MFRC522.h"
#include "rc52x.h"



int rc52x_get_chip_version(rc52x_t *rc52x, uint8_t *chip_id) {
	return mfrc522_recv(rc52x, RC52X_REG_VersionReg, chip_id, 1);
}

const char * rc52x_get_chip_name(rc52x_t *rc52x) {
	uint8_t chip_id = 0xFF;
	int result = rc52x_get_chip_version(rc52x, &chip_id);
	if (result) return "Error";
	switch (chip_id) {

	case 0x80:
		return "PN512 V1";
	case 0x82:

		return "PN512 V2";

		// Note: this is not in the datasheet
	case 0x88:
		return "FM17522";

		// TODO Chip ID for FM17550

	case 0x90:
		return "MFRC522 V0";
	case 0x91:
		return "MFRC522 V1";
	case 0x92:
		return "MFRC522 V2";

	case 0xB1:
		return "MRFC523 V1";
	case 0xB2:
		return "MRFC523 V2";

	default:
		return "Unknown";
	}
}

//------------
// Firmware data for self-test
// Reference values based on firmware version
// Hint: if needed, you can remove unused self-test data to save flash memory
//
// Version 0.0 (0x90)
// Philips Semiconductors; Preliminary Specification Revision 2.0 - 01 August 2005; 16.1 self-test
const uint8_t MFRC522_firmware_referenceV0_0[] = { 0x00, 0x87, 0x98, 0x0f, 0x49,
		0xFF, 0x07, 0x19, 0xBF, 0x22, 0x30, 0x49, 0x59, 0x63, 0xAD, 0xCA, 0x7F,
		0xE3, 0x4E, 0x03, 0x5C, 0x4E, 0x49, 0x50, 0x47, 0x9A, 0x37, 0x61, 0xE7,
		0xE2, 0xC6, 0x2E, 0x75, 0x5A, 0xED, 0x04, 0x3D, 0x02, 0x4B, 0x78, 0x32,
		0xFF, 0x58, 0x3B, 0x7C, 0xE9, 0x00, 0x94, 0xB4, 0x4A, 0x59, 0x5B, 0xFD,
		0xC9, 0x29, 0xDF, 0x35, 0x96, 0x98, 0x9E, 0x4F, 0x30, 0x32, 0x8D };
// Version 1.0 (0x91)
// NXP Semiconductors; Rev. 3.8 - 17 September 2014; 16.1.1 self-test
const uint8_t MFRC522_firmware_referenceV1_0[] = { 0x00, 0xC6, 0x37, 0xD5, 0x32,
		0xB7, 0x57, 0x5C, 0xC2, 0xD8, 0x7C, 0x4D, 0xD9, 0x70, 0xC7, 0x73, 0x10,
		0xE6, 0xD2, 0xAA, 0x5E, 0xA1, 0x3E, 0x5A, 0x14, 0xAF, 0x30, 0x61, 0xC9,
		0x70, 0xDB, 0x2E, 0x64, 0x22, 0x72, 0xB5, 0xBD, 0x65, 0xF4, 0xEC, 0x22,
		0xBC, 0xD3, 0x72, 0x35, 0xCD, 0xAA, 0x41, 0x1F, 0xA7, 0xF3, 0x53, 0x14,
		0xDE, 0x7E, 0x02, 0xD9, 0x0F, 0xB5, 0x5E, 0x25, 0x1D, 0x29, 0x79 };
// Version 2.0 (0x92)
// NXP Semiconductors; Rev. 3.8 - 17 September 2014; 16.1.1 self-test
const uint8_t MFRC522_firmware_referenceV2_0[] = { 0x00, 0xEB, 0x66, 0xBA, 0x57,
		0xBF, 0x23, 0x95, 0xD0, 0xE3, 0x0D, 0x3D, 0x27, 0x89, 0x5C, 0xDE, 0x9D,
		0x3B, 0xA7, 0x00, 0x21, 0x5B, 0x89, 0x82, 0x51, 0x3A, 0xEB, 0x02, 0x0C,
		0xA5, 0x00, 0x49, 0x7C, 0x84, 0x4D, 0xB3, 0xCC, 0xD2, 0x1B, 0x81, 0x5D,
		0x48, 0x76, 0xD5, 0x71, 0x61, 0x21, 0xA9, 0x86, 0x96, 0x83, 0x38, 0xCF,
		0x9D, 0x5B, 0x6D, 0xDC, 0x15, 0xBA, 0x3E, 0x7D, 0x95, 0x3B, 0x2F };

// TODO: Add references for MFRC523, PN512
// TODO: Add define guards for inclusion


/*

MFRC523 V1 (0xB1)
00h, C6h, 37h, D5h, 32h, B7h, 57h, 5Ch
C2h, D8h, 7Ch, 4Dh, D9h, 70h, C7h, 73h
10h, E6h, D2h, AAh, 5Eh, A1h, 3Eh, 5Ah
14h, AFh, 30h, 61h, C9h, 70h, DBh, 2Eh
64h, 22h, 72h, B5h, BDh, 65h, F4h, ECh
22h, BCh, D3h, 72h, 35h, CDh, AAh, 41h
1Fh, A7h, F3h, 53h, 14h, DEh, 7Eh, 02h
D9h, 0Fh, B5h, 5Eh, 25h, 1Dh, 29h, 79h

MFRC523 V2 (0xB2)
00h, EBh, 66h, BAh, 57h, BFh, 23h, 95h, D0h, E3h, 0Dh, 3Dh, 27h, 89h, 5Ch, DEh, 9Dh,
3Bh, A7h, 00h, 21h, 5Bh, 89h, 82h, 51h, 3Ah, EBh, 02h, 0Ch, A5h, 00h, 49h, 7Ch,
84h, 4Dh, B3h, CCh, D2h, 1Bh, 81h, 5Dh, 48h, 76h, D5h, 71h, 61h, 21h, A9h, 86h,
96h, 83h, 38h, CFh, 9Dh, 5Bh, 6Dh, DCh, 15h, BAh, 3Eh, 7Dh, 95h, 3Bh, 2Fh

PN512 V1 (0x80)
00h, AAh, E3h, 29h, 0Ch, 10h, 29zhh, 6Bh,
76h, 8Dh, AFh, 4Bh, A2h, DAh, 76h, 99h
C7h, 5Eh, 24h, 69h, D2h, BAh, FAh, BCh
3Eh, DAh, 96h, B5h, F5h, 94h, B0h, 3Ah
4Eh, C3h, 9Dh, 94h, 76h, 4Ch, EAh, 5Eh
38h, 10h, 8Fh, 2Dh, 21h, 4Bh, 52h, BFh
4Eh, C3h, 9Dh, 94h, 76h, 4Ch, EAh, 5Eh
38h, 10h, 8Fh, 2Dh, 21h, 4Bh, 52h, BFh
FBh, F4h, 19h, 94h, 82h, 5Ah, 72h, 9Dh
BAh, 0Dh, 1Fh, 17h, 56h, 22h, B9h, 08h

PN512 V2 (0x82)
00h, EBh, 66h, BAh, 57h, BFh, 23h, 95h, D0h, E3h, 0Dh, 3Dh, 27h, 89h, 5Ch, DEh,
9Dh, 3Bh, A7h, 00h, 21h, 5Bh, 89h, 82h, 51h, 3Ah, EBh, 02h, 0Ch, A5h, 00h,
49h, 7Ch, 84h, 4Dh, B3h, CCh, D2h, 1Bh, 81h, 5Dh, 48h, 76h, D5h, 71h, 61h,
21h, A9h, 86h, 96h, 83h, 38h, CFh, 9Dh, 5Bh, 6Dh, DCh, 15h, BAh, 3Eh, 7Dh,
95h, 3Bh, 2Fh

 */

// Clone
// Fudan Semiconductor FM17522 (0x88)
const uint8_t FM17522_firmware_reference[] = { 0x00, 0xD6, 0x78, 0x8C, 0xE2,
		0xAA, 0x0C, 0x18, 0x2A, 0xB8, 0x7A, 0x7F, 0xD3, 0x6A, 0xCF, 0x0B, 0xB1,
		0x37, 0x63, 0x4B, 0x69, 0xAE, 0x91, 0xC7, 0xC3, 0x97, 0xAE, 0x77, 0xF4,
		0x37, 0xD7, 0x9B, 0x7C, 0xF5, 0x3C, 0x11, 0x8F, 0x15, 0xC3, 0xD7, 0xC1,
		0x5B, 0x00, 0x2A, 0xD0, 0x75, 0xDE, 0x9E, 0x51, 0x64, 0xAB, 0x3E, 0xE9,
		0x15, 0xB5, 0xAB, 0x56, 0x9A, 0x98, 0x82, 0x26, 0xEA, 0x2A, 0x62 };


//--------------


// Temporary helper
// I don't like this style of function as it offers no
// possibility for error handling. Adding it to speed up portong
uint8_t RC52X_ReadRegister(rc52x_t *rc52x, uint8_t reg) {
	uint8_t regval;
	int result = rc52x_get_reg8(rc52x, reg, &regval);
	(void) result;  // We do not handle the error!
	return regval;
}

int RC52X_ClearRegisterBitMask(rc52x_t *rc52x, uint8_t reg, uint8_t value) {
	return rc52x_and_reg8(rc52x, reg, ~value);
}

/**
 * Use the CRC coprocessor in the MFRC522 to calculate a CRC_A.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t RC52X_CalculateCRC(rc52x_t *rc52x, uint8_t *data, ///< In: Pointer to the data to transfer to the FIFO for CRC calculation.
		uint8_t length,	///< In: The number of uint8_ts to transfer.
		uint8_t *result	///< Out: Pointer to result buffer. Result is written to result[0..1], low uint8_t first.
		) {
	rc52x_set_reg8(rc52x, RC52X_REG_CommandReg, RC52X_CMD_Idle);// Stop any active command.
	rc52x_set_reg8(rc52x, RC52X_REG_DivIrqReg, 0x04);// Clear the CRCIRq interrupt request bit
	rc52x_set_reg8(rc52x, RC52X_REG_FIFOLevelReg, 0x80);	// FlushBuffer = 1, FIFO initialization
	mfrc522_send(rc52x, RC52X_REG_FIFODataReg, data, length);// Write data to the FIFO
	rc52x_set_reg8(rc52x, RC52X_REG_CommandReg, RC52X_CMD_CalcCRC);// Start the calculation

	// Wait for the CRC calculation to complete. Each iteration of the while-loop takes 17.73μs.
	// TODO check/modify for other architectures than Arduino Uno 16bit

	// Wait for the CRC calculation to complete. Each iteration of the while-loop takes 17.73us.
	for (uint16_t i = 5000; i > 0; i--) {
		// DivIrqReg[7..0] bits are: Set2 reserved reserved MfinActIRq reserved CRCIRq reserved reserved
		uint8_t n = RC52X_ReadRegister(rc52x, RC52X_REG_DivIrqReg);
		if (n & 0x04) {						// CRCIRq bit set - calculation done
			rc52x_set_reg8(rc52x, RC52X_REG_CommandReg, RC52X_CMD_Idle);// Stop calculating CRC for new content in the FIFO.
			// Transfer the result from the registers to the result buffer
			result[0] = RC52X_ReadRegister(rc52x, RC52X_REG_CRCResultReg_Lo);
			result[1] = RC52X_ReadRegister(rc52x, RC52X_REG_CRCResultReg_Hi);
			return STATUS_OK;
		}
	}
	// 89ms passed and nothing happend. Communication with the MFRC522 might be down.
	return STATUS_TIMEOUT;
} // End RC52X_CalculateCRC()

/////////////////////////////////////////////////////////////////////////////////////
// Functions for manipulating the MFRC522
/////////////////////////////////////////////////////////////////////////////////////






/**
 * Initializes the MFRC522 chip.
 */
void RC52X_Init(rc52x_t *rc52x) {
	rc52x->TransceiveData = RC52X_TransceiveData;
	//rc52x->SetBitFraming = rc52x_set_bit_framing;
	RC52X_Reset(rc52x);

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

	// Fix for PN512 compatibility:
	// Set Bit 4 in ControlReg: Set to logic 1, the PN512 acts as initiator, otherwise it acts as target
	rc52x_set_reg8(rc52x, RC52X_REG_ControlReg, 0x10);

	RC52X_AntennaOn(rc52x);// Enable the antenna driver pins TX1 and TX2 (they were disabled by the reset)
} // End RC52X_Init()

/**
 * Performs a soft reset on the MFRC522 chip and waits for it to be ready again.
 */
void RC52X_Reset(rc52x_t *rc52x) {
	rc52x_set_reg8(rc52x, RC52X_REG_CommandReg, RC52X_CMD_SoftReset); // Issue the SoftReset command.
	// The datasheet does not mention how long the SoftRest command takes to complete.
	// But the MFRC522 might have been in soft power-down mode (triggered by bit 4 of CommandReg)
	// Section 8.8.2 in the datasheet says the oscillator start-up time is the start up time of the crystal + 37,74μs. Let us be generous: 50ms.
	uint8_t count = 0;
	do {
		// Wait for the PowerDown bit in CommandReg to be cleared (max 3x50ms)
		rc52x->delay_ms(50);

	} while ((RC52X_ReadRegister(rc52x, RC52X_REG_CommandReg) & (1 << 4))
			&& (++count) < 3);
} // End RC52X_Reset()

/**
 * Turns the antenna on by enabling pins TX1 and TX2.
 * After a reset these pins are disabled.
 */
void RC52X_AntennaOn(rc52x_t *rc52x) {
//	uint8_t value = RC52X_ReadRegister(rc52x, RC52X_REG_TxControlReg);
//	if ((value & 0x03) != 0x03) {
//		rc52x_set_reg8(rc52x, RC52X_REG_TxControlReg, value | 0x03);
//	}

	rc52x_set_reg8(rc52x, RC52X_REG_TxControlReg, 0x83);
} // End RC52X_AntennaOn()

/**
 * Turns the antenna off by disabling pins TX1 and TX2.
 */
void RC52X_AntennaOff(rc52x_t *rc52x) {
	//RC52X_ClearRegisterBitMask(rc52x, RC52X_REG_TxControlReg, 0x03);
	rc52x_set_reg8(rc52x, RC52X_REG_TxControlReg, 0x80);
} // End RC52X_AntennaOff()

/**
 * Get the current MFRC522 Receiver Gain (RxGain[2:0]) value.
 * See 9.3.3.6 / table 98 in http://www.nxp.com/documents/data_sheet/MFRC522.pdf
 * NOTE: Return value scrubbed with (0x07<<4)=01110000b as RCFfgReg may use reserved bits.
 *
 * @return Value of the RxGain, scrubbed to the 3 bits used.
 */
uint8_t RC52X_GetAntennaGain(rc52x_t *rc52x) {
	return RC52X_ReadRegister(rc52x, RC52X_REG_RFCfgReg) & (0x07 << 4);
} // End RC52X_GetAntennaGain()

/**
 * Set the MFRC522 Receiver Gain (RxGain) to value specified by given mask.
 * See 9.3.3.6 / table 98 in http://www.nxp.com/documents/data_sheet/MFRC522.pdf
 * NOTE: Given mask is scrubbed with (0x07<<4)=01110000b as RCFfgReg may use reserved bits.
 */
void RC52X_SetAntennaGain(rc52x_t *rc52x, uint8_t mask) {
	if (RC52X_GetAntennaGain(rc52x) != mask) { // only bother if there is a change
		RC52X_ClearRegisterBitMask(rc52x, RC52X_REG_RFCfgReg, (0x07 << 4)); // clear needed to allow 000 pattern
		RC52X_SetRegisterBitMask(rc52x, RC52X_REG_RFCfgReg, mask & (0x07 << 4)); // only set RxGain[2:0] bits
	}
} // End RC52X_SetAntennaGain()

/**
 * Performs a self-test of the MFRC522
 * See 16.1.1 in http://www.nxp.com/documents/data_sheet/MFRC522.pdf
 *
 * @return Whether or not the test passed. Or false if no firmware reference is available.
 */
bool RC52X_PerformSelfTest(rc52x_t *rc52x) {
	// This follows directly the steps outlined in 16.1.1
	// 1. Perform a soft reset.
	RC52X_Reset(rc52x);

	// 2. Clear the internal buffer by writing 25 uint8_ts of 00h
	uint8_t ZEROES[25] = { 0x00 };
	rc52x_set_reg8(rc52x, RC52X_REG_FIFOLevelReg, 0x80);	// flush the FIFO buffer
	mfrc522_send(rc52x, RC52X_REG_FIFODataReg, ZEROES, 25);// write 25 uint8_ts of 00h to FIFO
	rc52x_set_reg8(rc52x, RC52X_REG_CommandReg, RC52X_CMD_Configure);	// transfer to internal buffer

	// 3. Enable self-test
	rc52x_set_reg8(rc52x, RC52X_REG_AutoTestReg, 0x09);

	// 4. Write 00h to FIFO buffer
	rc52x_set_reg8(rc52x, RC52X_REG_FIFODataReg, 0x00);

	// 5. Start self-test by issuing the CalcCRC command
	rc52x_set_reg8(rc52x, RC52X_REG_CommandReg, RC52X_CMD_CalcCRC);

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
		n = RC52X_ReadRegister(rc52x, RC52X_REG_FIFOLevelReg);
		if (n >= 64) {
			break;
		}
	}
	rc52x_set_reg8(rc52x, RC52X_REG_CommandReg, RC52X_CMD_Idle);// Stop calculating CRC for new content in the FIFO.

	// 7. Read out resulting 64 uint8_ts from the FIFO buffer.
	uint8_t result[64];
	mfrc522_recv(rc52x, RC52X_REG_FIFODataReg, result, 64);

	// Auto self-test done
	// Reset AutoTestReg register to be 0 again. Required for normal operation.
	rc52x_set_reg8(rc52x, RC52X_REG_AutoTestReg, 0x00);

	// Determine firmware version (see section 9.3.4.8 in spec)
	uint8_t version = RC52X_ReadRegister(rc52x, RC52X_REG_VersionReg);

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
} // End RC52X_PerformSelfTest()

/////////////////////////////////////////////////////////////////////////////////////
// Power control
/////////////////////////////////////////////////////////////////////////////////////

//IMPORTANT NOTE!!!!
//Calling any other function that uses CommandReg will disable soft power down mode !!!
//For more details about power control, refer to the datasheet - page 33 (8.6)

void RC52X_SoftPowerDown(rc52x_t *rc52x) { //Note : Only soft power down mode is available throught software
	uint8_t val = RC52X_ReadRegister(rc52x, RC52X_REG_CommandReg); // Read state of the command register
	val |= (1 << 4); // set PowerDown bit ( bit 4 ) to 1
	rc52x_set_reg8(rc52x, RC52X_REG_CommandReg, val); //write new value to the command register
}

void RC52X_SoftPowerUp(rc52x_t *rc52x) {
	uint8_t val = RC52X_ReadRegister(rc52x, RC52X_REG_CommandReg); // Read state of the command register
	val &= ~(1 << 4); // set PowerDown bit ( bit 4 ) to 0
	rc52x_set_reg8(rc52x, RC52X_REG_CommandReg, val); //write new value to the command register
	// wait until PowerDown bit is cleared (this indicates end of wake up procedure)
	const uint32_t timeout = (uint32_t) millis() + 500;	// create timer for timeout (just in case)

	while (millis() <= timeout) { // set timeout to 500 ms
		val = RC52X_ReadRegister(rc52x, RC52X_REG_CommandReg); // Read state of the command register
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
rc52x_result_t RC52X_TransceiveData(rc52x_t *rc52x, uint8_t *sendData, ///< Pointer to the data to transfer to the FIFO.
		size_t sendLen,		///< Number of uint8_ts to transfer to the FIFO.
		uint8_t *backData,///< nullptr or pointer to buffer if data should be read back after executing the command.
		size_t *backLen,///< In: Max number of uint8_ts to write to *backData. Out: The number of uint8_ts returned.
		uint8_t *validBits,	///< In/Out: The number of valid bits in the last uint8_t. 0 for 8 valid bits. Default nullptr.
		uint8_t rxAlign,///< In: Defines the bit position in backData[0] for the first bit received. Default 0.
		uint8_t *collisionPos,
		bool sendCRC ,
		bool recvCRC
		) {
	uint8_t waitIRq = 0x30;		// RxIRq and IdleIRq
	return RC52X_CommunicateWithPICC(rc52x, RC52X_CMD_Transceive, waitIRq,
			sendData, sendLen, backData, backLen, validBits, rxAlign, collisionPos, sendCRC, recvCRC);
} // End RC52X_TransceiveData()

/**
 * Transfers data to the MFRC522 FIFO, executes a command, waits for completion and transfers data back from the FIFO.
 * CRC validation can only be done if backData and backLen are specified.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t RC52X_CommunicateWithPICC(rc52x_t *rc52x, uint8_t command,	///< The command to execute. One of the RC52X_Command enums.
		uint8_t waitIRq,///< The bits in the ComIrqReg register that signals successful completion of the command.
		uint8_t *sendData,	///< Pointer to the data to transfer to the FIFO.
		size_t sendLen,		///< Number of uint8_ts to transfer to the FIFO.
		uint8_t *backData,///< nullptr or pointer to buffer if data should be read back after executing the command.
		size_t *backLen,///< In: Max number of uint8_ts to write to *backData. Out: The number of uint8_ts returned.
		uint8_t *validBits,	///< In/Out: The number of valid bits in the last uint8_t. 0 for 8 valid bits.
		uint8_t rxAlign,///< In: Defines the bit position in backData[0] for the first bit received. Default 0.
		uint8_t *collisionPos,
		bool sendCRC ,
		bool recvCRC
		) {
	// Prepare values for BitFramingReg
	uint8_t txLastBits = validBits ? *validBits : 0;
	uint8_t bitFraming = (rxAlign << 4) + txLastBits;// RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]



	if (sendCRC) {
		rc52x_result_t result = RC52X_CalculateCRC(rc52x, sendData, sendLen,
				&sendData[sendLen]);
		if (result != STATUS_OK) {
			return result;
		}
		sendLen += 2;
	}

	RC52X_ClearRegisterBitMask(rc52x, RC52X_REG_CollReg, 0x80);


	rc52x_set_reg8(rc52x, RC52X_REG_CommandReg, RC52X_CMD_Idle);// Stop any active command.
	rc52x_set_reg8(rc52x, RC52X_REG_ComIrqReg, 0x7F);// Clear all seven interrupt request bits
	rc52x_set_reg8(rc52x, RC52X_REG_FIFOLevelReg, 0x80);	// FlushBuffer = 1, FIFO initialization
	mfrc522_send(rc52x, RC52X_REG_FIFODataReg, sendData, sendLen);// Write sendData to the FIFO
	rc52x_set_reg8(rc52x, RC52X_REG_BitFramingReg, bitFraming);// Bit adjustments
	rc52x_set_reg8(rc52x, RC52X_REG_CommandReg, command);// Execute the command
	if (command == RC52X_CMD_Transceive) {
		//rc52x_set_reg8(rc52x, RC52X_REG_BitFramingReg, 0x80);// StartSend=1, transmission of data starts
		rc52x_or_reg8(rc52x, RC52X_REG_BitFramingReg, 0x80);	// StartSend=1, transmission of data starts
	}

	// Wait for the command to complete.
	// In RC52X_Init() we set the TAuto flag in TModeReg. This means the timer automatically starts when the PCD stops transmitting.
	// Each iteration of the do-while-loop takes 17.86μs.
	// TODO check/modify for other architectures than Arduino Uno 16bit
	uint16_t i;
	uint8_t n;

	for (i = 2000; i > 0; i--) {
		n = RC52X_ReadRegister(rc52x, RC52X_REG_ComIrqReg);// ComIrqReg[7..0] bits are: Set1 TxIRq RxIRq IdleIRq HiAlertIRq LoAlertIRq ErrIRq TimerIRq
		if (n & waitIRq) {// One of the interrupts that signal success has been set.
			break;
		}
		if (n & 0x01) {			// Timer interrupt - nothing received in 25ms
			return STATUS_TIMEOUT;
		}
		if (n & 0x02) {			// Timer interrupt - nothing received in 25ms
			return STATUS_ERROR;
		}

	}
	// 35.7ms and nothing happend. Communication with the MFRC522 might be down.
	// WHy 35.7ms? Is this on some hardware timing for the original whatever this was developed on
	// Using the 2000 iterations loop?
	if (i == 0) {


		uint8_t errorRegValue = RC52X_ReadRegister(rc52x, RC52X_REG_ErrorReg); // ErrorReg[7..0] bits are: WrErr TempErr reserved BufferOvfl CollErr CRCErr ParityErr ProtocolErr
		(void)errorRegValue;
		return STATUS_TIMEOUT;
	}

	// Stop now if any errors except collisions were detected.
	uint8_t errorRegValue = RC52X_ReadRegister(rc52x, RC52X_REG_ErrorReg); // ErrorReg[7..0] bits are: WrErr TempErr reserved BufferOvfl CollErr CRCErr ParityErr ProtocolErr
	if (errorRegValue & 0x13) {	 // BufferOvfl ParityErr ProtocolErr
		return STATUS_ERROR;
	}

	uint8_t _validBits = 0;

	// If the caller wants data back, get it from the MFRC522.
	if (backData && backLen) {
		uint8_t n = RC52X_ReadRegister(rc52x, RC52X_REG_FIFOLevelReg);	// Number of uint8_ts in the FIFO
		if (n > *backLen) {
			return STATUS_NO_ROOM;
		}
		*backLen = n;							// Number of uint8_ts returned
		mfrc522_recv(rc52x, RC52X_REG_FIFODataReg, backData, n);
		_validBits = RC52X_ReadRegister(rc52x, RC52X_REG_ControlReg) & 0x07;// RxLastBits[2:0] indicates the number of valid bits in the last received uint8_t. If this value is 000b, the whole uint8_t is valid.
		if (validBits) {
			*validBits = _validBits;
		}
	}

	// Tell about collisions
	if (errorRegValue & 0x08) {		// CollErr
		if (!collisionPos)
			return STATUS_COLLISION;

		uint8_t valueOfCollReg = RC52X_ReadRegister(rc52x, RC52X_REG_CollReg); // CollReg[7..0] bits are: ValuesAfterColl reserved CollPosNotValid CollPos[4:0]
		if (valueOfCollReg & 0x20) { // CollPosNotValid
			*collisionPos = -1;
			return STATUS_COLLISION; // Without a valid collision position we cannot continue
		}

		*collisionPos = valueOfCollReg & 0x1F; // Values 0-31, 0 means bit 32.

		if (*collisionPos == 0) {
			*collisionPos = 32;
		}

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
	}

	return STATUS_OK;
} // End RC52X_CommunicateWithPICC()



/////////////////////////////////////////////////////////////////////////////////////
// Functions for communicating with MIFARE PICCs
/////////////////////////////////////////////////////////////////////////////////////

/**
 * Executes the MFRC522 MFAuthent command.
 * This command manages MIFARE authentication to enable a secure communication to any MIFARE Mini, MIFARE 1K and MIFARE 4K card.
 * The authentication is described in the MFRC522 datasheet section 10.3.1.9 and http://www.nxp.com/documents/data_sheet/MF1S503x.pdf section 10.1.
 * For use with MIFARE Classic PICCs.
 * The PICC must be selected - ie in state ACTIVE(*) - before calling this function.
 * Remember to call RC52X_StopCrypto1() after communicating with the authenticated PICC - otherwise no new communications can start.
 *
 * All keys are set to FFFFFFFFFFFFh at chip delivery.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise. Probably STATUS_TIMEOUT if you supply the wrong key.
 */
rc52x_result_t RC52X_Authenticate(rc52x_t *rc52x, uint8_t command, ///< PICC_CMD_MF_AUTH_KEY_A or PICC_CMD_MF_AUTH_KEY_B
		uint8_t blockAddr, ///< The block number. See numbering in the comments in the .h file.
		MIFARE_Key *key,	///< Pointer to the Crypteo1 key to use (6 uint8_ts)
		picc_t *picc///< Pointer to Uid struct. The first 4 uint8_ts of the UID is used.
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
	// but it requires cascade tag (CT) uint8_t, that is not part of picc.
	for (uint8_t i = 0; i < 4; i++) {		// The last 4 uint8_ts of the UID
		sendData[8 + i] = picc->uidByte[i + picc->size - 4];
	}

	// Start the authentication.
	return RC52X_CommunicateWithPICC(rc52x, RC52X_CMD_MFAuthent, waitIRq,
			&sendData[0], sizeof(sendData), NULL, 0, NULL, 0, NULL, false, false);
} // End RC52X_Authenticate()

/**
 * Used to exit the PCD from its authenticated state.
 * Remember to call this function after communicating with an authenticated PICC - otherwise no new communications can start.
 */
void RC52X_StopCrypto1(rc52x_t *rc52x) {
	// Clear MFCrypto1On bit
	RC52X_ClearRegisterBitMask(rc52x, RC52X_REG_Status2Reg, 0x08); // Status2Reg[7..0] bits are: TempSensClear I2CForceHS reserved reserved MFCrypto1On ModemState[2:0]
} // End RC52X_StopCrypto1()


/////////////////////////////////////////////////////////////////////////////////////
// Support functions
/////////////////////////////////////////////////////////////////////////////////////

/**
 * Wrapper for MIFARE protocol communication.
 * Adds CRC_A, executes the Transceive command and checks that the response is MF_ACK or a timeout.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t RC52X_MIFARE_Transceive(rc52x_t *rc52x, uint8_t *sendData,	///< Pointer to the data to transfer to the FIFO. Do NOT include the CRC_A.
		uint8_t sendLen,		///< Number of uint8_ts in sendData.
		bool acceptTimeout	///< True => A timeout is also success
		) {
	rc52x_result_t result;
	uint8_t cmdBuffer[18]; // We need room for 16 uint8_ts data and 2 uint8_ts CRC_A.

	// Sanity check
	if (sendData == NULL || sendLen > 16) {
		return STATUS_INVALID;
	}

	/*// moved into CommunicateWithPICC
	// Copy sendData[] to cmdBuffer[] and add CRC_A
	memcpy(cmdBuffer, sendData, sendLen);
	result = RC52X_CalculateCRC(rc52x, cmdBuffer, sendLen, &cmdBuffer[sendLen]);
	if (result != STATUS_OK) {
		return result;
	}
	sendLen += 2;
	*/


	// Transceive the data, store the reply in cmdBuffer[]
	uint8_t waitIRq = 0x30;		// RxIRq and IdleIRq
	uint8_t cmdBufferSize = sizeof(cmdBuffer);
	uint8_t validBits = 0;
	result = RC52X_CommunicateWithPICC(rc52x, RC52X_CMD_Transceive, waitIRq,
			cmdBuffer, sendLen, cmdBuffer, &cmdBufferSize, &validBits, 0, NULL,
			true, false );
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
} // End RC52X_MIFARE_Transceive()


rc52x_result_t rc52x_set_bit_framing(bs_pdc_t*pdc, int rxAlign, int txLastBits) {
	return rc52x_set_reg8(pdc, RC52X_REG_BitFramingReg, 	(rxAlign << 4) | txLastBits);// RxAlign = BitFramingReg[6..4]. TxLastBits = BitFramingReg[2..0]
}
