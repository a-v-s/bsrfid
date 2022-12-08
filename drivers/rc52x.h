/******************************************************************************
 File:         rc52x.h
 Author:       André van Schoubroeck
 License:      MIT

 This implements the RC522 family of RFID reader ICs

 * NXP MFRC522         (ISO/IEC14443A)
 * NXP MFRC523         (ISO/IEC14443A,ISO/IEC14443B)
 * NXP PN512           (ISO/IEC14443A,ISO/IEC14443B, FeliCa)
 * FUDAN FM17522E      (ISO/IEC14443A)
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


 MFRC522 only supports ISO 14443-A
 MFRC523 also supports ISO 14443-A, ISO 14443-B
 PN512 also supports ISO 14443-A, ISO 14443-B, JIS X 6319-4

 As the MFRC522 is the commonly available hardware on eBay and AliExpress, that
 is the hardware I have available. Support for the features of the other chips
 will be limited due missing hardware.

 *******************************************************************************/

/**
 * Based upon "Library to use Arduino MFRC522 module." https://github.com/miguelbalboa/rfid
 *
 * @authors Dr.Leong, Miguel Balboa, Søren Thing Andersen, Tom Clement, many more! See GitLog.
 *
 * For more information read the README.
 *
 * Please read this file for an overview and then MFRC522.cpp for comments on the specific functions.
 */

#ifndef _MFCR522_H_
#define _MFCR522_H_

#include "picc.h"
#include "iso14443a.h"
#include "iso14443b.h"

#include "rc52x_transport.h"

#include "pdc.h"
typedef bs_pdc_t rc52x_t;

//------------------------------------------------------------------------------
// Regisers
// -----------------------------------------------------------------------------
// These are the registers for all of RC522, RC523 and PN512. Some registers
// may be reserved on some of these chips.
// -----------------------------------------------------------------------------

#define RC52X_REG_CommandReg         (0x01)
#define RC52X_REG_ComlEnReg          (0x02)
#define RC52X_REG_DivlEnReg          (0x03)
#define RC52X_REG_ComIrqReg          (0x04)
#define RC52X_REG_DivIrqReg          (0x05)
#define RC52X_REG_ErrorReg           (0x06)
#define RC52X_REG_Status1Reg         (0x07)
#define RC52X_REG_Status2Reg         (0x08)
#define RC52X_REG_FIFODataReg        (0x09)
#define RC52X_REG_FIFOLevelReg       (0x0A)
#define RC52X_REG_WaterLevelReg      (0x0B)
#define RC52X_REG_ControlReg         (0x0C)
#define RC52X_REG_BitFramingReg      (0x0D)
#define RC52X_REG_CollReg            (0x0E)

#define RC52X_REG_ModeReg            (0x11)
#define RC52X_REG_TxModeReg          (0x12)
#define RC52X_REG_RxModeReg          (0x13)
#define RC52X_REG_TxControlReg       (0x14)
#define RC52X_REG_TxASKReg           (0x15)
#define RC52X_REG_TxSelReg           (0x16)
#define RC52X_REG_RxSelReg           (0x17)
#define RC52X_REG_RxThresholdReg     (0x18)
#define RC52X_REG_DemodReg           (0x19)
#define RC52X_REG_FelNFC1Reg         (0x1A)
#define RC52X_REG_FelNFC2Reg         (0x1B)
#define RC52X_REG_MfTxReg            (0x1C)
#define RC52X_REG_MfRxReg            (0x1D)
#define RC52X_REG_TypeBReg           (0x1E)
#define RC52X_REG_SerialSpeedReg     (0x1F)

#define RC52X_REG_CRCResultReg_Hi    (0x21)
#define RC52X_REG_CRCResultReg_Lo    (0x22)
#define RC52X_REG_GsNOffReg          (0x23)
#define RC52X_REG_ModWidthReg        (0x24)
#define RC52X_REG_TxBitPhaseReg      (0x25)
#define RC52X_REG_RFCfgReg           (0x26)
#define RC52X_REG_GsNOnReg           (0x27)
#define RC52X_REG_CWGsPReg           (0x28)
#define RC52X_REG_ModGsPReg          (0x29)
#define RC52X_REG_TModeReg           (0x2A)
#define RC52X_REG_TPrescalerReg      (0x2B)
#define RC52X_REG_TReloadReg_Hi      (0x2C)
#define RC52X_REG_TReloadReg_Lo      (0x2D)
#define RC52X_REG_TCounterVal_Hi     (0x2E)
#define RC52X_REG_TCounterVal_Lo     (0x2F)

#define RC52X_REG_TestSel1Reg        (0x31)
#define RC52X_REG_TestSel2Reg        (0x32)
#define RC52X_REG_TestPinEnReg       (0x33)
#define RC52X_REG_TestPinValueReg    (0x34)
#define RC52X_REG_TestBusReg         (0x35)
#define RC52X_REG_AutoTestReg        (0x36)
#define RC52X_REG_VersionReg         (0x37)
#define RC52X_REG_AnalogTestReg      (0x38)
#define RC52X_REG_TestDAC1Reg        (0x39)
#define RC52X_REG_TestDAC2Reg        (0x3A)
#define RC52X_REG_TestADCReg         (0x3B)

//------------------------------------------------------------------------------
// Register Helper
// -----------------------------------------------------------------------------
// Define sequential mode and 16 bit registers. (NB Big Endian) 
// -----------------------------------------------------------------------------

#define RC52X_REG_SequentialAddr     (0x40)
#define RC52X_REG_CRCResultReg       (MFRC_SequentialAddr + MFRC_CRCResultReg_Hi)
#define RC52X_REG_TCounterVal        (MFRC_SequentialAddr + MFRC_TCounterVal_Hi)
#define RC52X_REG_TReloadReg			(MFRC_SequentialAddr + RC52X_REG_TReloadReg_Hi)

//------------------------------------------------------------------------------
// Commands
// -----------------------------------------------------------------------------
// These are the commands for all of RC522, RC523 and PN512. Some commands
// may be reserved on some of these chips.
// -----------------------------------------------------------------------------

#define RC52X_CMD_Idle               (0b0000)
#define RC52X_CMD_Configure          (0b0001)
#define RC52X_CMD_GenerateRandomID   (0b0010)
#define RC52X_CMD_CalcCRC            (0b0011)
#define RC52X_CMD_Transmit           (0b0100)
#define RC52X_CMD_NoCmdChange        (0b0111)
#define RC52X_CMD_Receive            (0b1000)
#define RC52X_CMD_Transceive         (0b1100)
#define RC52X_CMD_AutoColl           (0b1101)
#define RC52X_CMD_MFAuthent          (0b1110)
#define RC52X_CMD_SoftReset          (0b1111)

#define RC52X_TIMEOUT_ms			(40)

uint8_t rc52x_communicate_with_picc(rc52x_t *rc52x, uint8_t command, ///< The command to execute. One of the RC52X_Command enums.
		uint8_t waitIRq, ///< The bits in the ComIrqReg register that signals successful completion of the command.
		uint8_t *sendData,	///< Pointer to the data to transfer to the FIFO.
		uint8_t sendLen,		///< Number of bytes to transfer to the FIFO.
		uint8_t *backData,///< NULL or pointer to buffer if data should be read back after executing the command.
		uint8_t *backLen,///< In: Max number of bytes to write to *backData. Out: The number of bytes returned.
		uint8_t *validBits,	///< In/Out: The number of valid bits in the last byte. 0 for 8 valid bits.
		uint8_t rxAlign ///< In: Defines the bit position in backData[0] for the first bit received. Default 0.
		);

int mfrc522_send(rc52x_t *rc52x, uint8_t reg, uint8_t *data, size_t amount);
int rc52x_set_reg8(rc52x_t *rc52x, uint8_t reg, uint8_t value);
int rc52x_get_reg8(rc52x_t *rc52x, uint8_t reg, uint8_t *value);
int rc52x_and_reg8(rc52x_t *rc52x, uint8_t reg, uint8_t value);
int rc52x_or_reg8(rc52x_t *rc52x, uint8_t reg, uint8_t value);

void rc52x_init(rc52x_t *rc52x) ;


rc52x_result_t rc52x_set_bit_framing(bs_pdc_t *pdc, int rxAlign,
		int txLastBits);

int mfrc522_recv(rc52x_t *rc52x, uint8_t reg, uint8_t *data, size_t amount);
int mfrc522_send(rc52x_t *rc52x, uint8_t reg, uint8_t *data, size_t amount);
void rc52x_reset(rc52x_t *rc52x);
int rc52x_get_chip_version(rc52x_t *rc52x, uint8_t *chip_id);
const char* rc52x_get_chip_name(rc52x_t *rc52x);



///---
//  Below are old style function names from before the port
/// these have to be removed:
///---

// MFRC522 RxGain[2:0] masks, defines the receiver's signal voltage gain factor (on the PCD).
// Described in 9.3.3.6 / table 98 of the datasheet at http://www.nxp.com/documents/data_sheet/MFRC522.pdf
enum RC52X_RxGain {
	RxGain_18dB = 0x00 << 4,	// 000b - 18 dB, minimum
	RxGain_23dB = 0x01 << 4,	// 001b - 23 dB
	RxGain_18dB_2 = 0x02 << 4,// 010b - 18 dB, it seems 010b is a duplicate for 000b
	RxGain_23dB_2 = 0x03 << 4,// 011b - 23 dB, it seems 011b is a duplicate for 001b
	RxGain_33dB = 0x04 << 4,	// 100b - 33 dB, average, and typical default
	RxGain_38dB = 0x05 << 4,	// 101b - 38 dB
	RxGain_43dB = 0x06 << 4,	// 110b - 43 dB
	RxGain_48dB = 0x07 << 4,	// 111b - 48 dB, maximum
	RxGain_min = 0x00 << 4,	// 000b - 18 dB, minimum, convenience for RxGain_18dB
	RxGain_avg = 0x04 << 4,	// 100b - 33 dB, average, convenience for RxGain_33dB
	RxGain_max = 0x07 << 4// 111b - 48 dB, maximum, convenience for RxGain_48dB
};

// MIFARE constants that does not fit anywhere else
enum MIFARE_Misc {
	MF_ACK = 0xA,// The MIFARE Classic uses a 4 bit ACK/NAK. Any other value than 0xA is NAK.
	MF_KEY_SIZE = 6			// A Mifare Crypto1 key is 6 uint8_ts.
};

// PICC types we can detect. Remember to update PICC_GetTypeName() if you add more.
// last value set to 0xff, then compiler uses less ram, it seems some optimisations are triggered
enum PICC_Type {
	PICC_TYPE_UNKNOWN, PICC_TYPE_ISO_14443_4,// PICC compliant with ISO/IEC 14443-4
	PICC_TYPE_ISO_18092, 	// PICC compliant with ISO/IEC 18092 (NFC)
	PICC_TYPE_MIFARE_MINI,	// MIFARE Classic protocol, 320 uint8_ts
	PICC_TYPE_MIFARE_1K,	// MIFARE Classic protocol, 1KB
	PICC_TYPE_MIFARE_4K,	// MIFARE Classic protocol, 4KB
	PICC_TYPE_MIFARE_UL,	// MIFARE Ultralight or Ultralight C
	PICC_TYPE_MIFARE_PLUS,	// MIFARE Plus
	PICC_TYPE_MIFARE_DESFIRE,	// MIFARE DESFire
	PICC_TYPE_TNP3XXX,// Only mentioned in NXP AN 10833 MIFARE Type Identification Procedure
	PICC_TYPE_NOT_COMPLETE = 0xff	// SAK indicates UID is not complete.
};

// A struct used for passing a MIFARE Crypto1 key
typedef struct {
	uint8_t keyuint8_t[MF_KEY_SIZE];
} MIFARE_Key;

/////////////////////////////////////////////////////////////////////////////////////
// Basic interface functions for communicating with the MFRC522
/////////////////////////////////////////////////////////////////////////////////////
rc52x_result_t RC52X_CalculateCRC(rc52x_t *rc52x, uint8_t *data, uint8_t length,
		uint8_t *result);

/////////////////////////////////////////////////////////////////////////////////////
// Functions for manipulating the MFRC522
/////////////////////////////////////////////////////////////////////////////////////
void RC52X_Init(rc52x_t *rc52x);
void RC52X_Reset(rc52x_t *rc52x);
void rc52x_antenna_on(rc52x_t *rc52x);
void RC52X_AntennaOff(rc52x_t *rc52x);
uint8_t RC52X_GetAntennaGain(rc52x_t *rc52x);
void RC52X_SetAntennaGain(rc52x_t *rc52x, uint8_t mask);
bool RC52X_PerformSelfTest(rc52x_t *rc52x);

/////////////////////////////////////////////////////////////////////////////////////
// Power control functions
/////////////////////////////////////////////////////////////////////////////////////
void RC52X_SoftPowerDown(rc52x_t *rc52x);
void RC52X_SoftPowerUp(rc52x_t *rc52x);

/////////////////////////////////////////////////////////////////////////////////////
// Functions for communicating with PICCs
/////////////////////////////////////////////////////////////////////////////////////
rc52x_result_t rc52x_transceive(rc52x_t *rc52x, uint8_t *sendData,
		size_t sendLen, uint8_t *backData, size_t *backLen, uint8_t *validBits,
		uint8_t rxAlign, uint8_t *collisionPos, bool sendCRC, bool recvCRC);
rc52x_result_t RC52X_CommunicateWithPICC(rc52x_t *rc52x, uint8_t command,
		uint8_t waitIRq, uint8_t *sendData, size_t sendLen, uint8_t *backData,
		size_t *backLen, uint8_t *validBits, uint8_t rxAlign,
		uint8_t *collisionPos, bool sendCRC, bool recvCRC);

rc52x_result_t PICC_RequestA(rc52x_t *rc52x, picc_t *picc);
rc52x_result_t PICC_WakeupA(rc52x_t *rc52x, picc_t *picc);
rc52x_result_t PICC_REQA_or_WUPA(rc52x_t *rc52x, uint8_t command,
		uint8_t *bufferATQA, size_t *bufferSize);
rc52x_result_t PICC_Select(rc52x_t *rc52x, picc_t *uid, uint8_t validBits);
rc52x_result_t PICC_HaltA(rc52x_t *rc52x);

///---



#endif // _MFCR522_H_

