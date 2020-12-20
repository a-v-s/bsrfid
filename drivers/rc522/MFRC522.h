/**
 * Library to use Arduino MFRC522 module.
 * 
 * @authors Dr.Leong, Miguel Balboa, Søren Thing Andersen, Tom Clement, many more! See GitLog.
 * 
 * For more information read the README.
 * 
 * Please read this file for an overview and then MFRC522.cpp for comments on the specific functions.
 */
#ifndef MFRC522_h
#define MFRC522_h

#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include "rc52x.h"

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
// Clone
// Fudan Semiconductor FM17522 (0x88)
const uint8_t FM17522_firmware_reference[] = { 0x00, 0xD6, 0x78, 0x8C, 0xE2,
		0xAA, 0x0C, 0x18, 0x2A, 0xB8, 0x7A, 0x7F, 0xD3, 0x6A, 0xCF, 0x0B, 0xB1,
		0x37, 0x63, 0x4B, 0x69, 0xAE, 0x91, 0xC7, 0xC3, 0x97, 0xAE, 0x77, 0xF4,
		0x37, 0xD7, 0x9B, 0x7C, 0xF5, 0x3C, 0x11, 0x8F, 0x15, 0xC3, 0xD7, 0xC1,
		0x5B, 0x00, 0x2A, 0xD0, 0x75, 0xDE, 0x9E, 0x51, 0x64, 0xAB, 0x3E, 0xE9,
		0x15, 0xB5, 0xAB, 0x56, 0x9A, 0x98, 0x82, 0x26, 0xEA, 0x2A, 0x62 };

// Size of the MFRC522 FIFO
static uint8_t FIFO_SIZE = 64;		// The FIFO is 64 uint8_ts.
// Default value for unused pin
static uint8_t UNUSED_PIN = UINT8_MAX;

// MFRC522 RxGain[2:0] masks, defines the receiver's signal voltage gain factor (on the PCD).
// Described in 9.3.3.6 / table 98 of the datasheet at http://www.nxp.com/documents/data_sheet/MFRC522.pdf
enum PCD_RxGain {
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

// Commands sent to the PICC.
enum PICC_Command {
	// The commands used by the PCD to manage communication with several PICCs (ISO 14443-3, Type A, section 6.4)
	PICC_CMD_REQA = 0x26,// REQuest command, Type A. Invites PICCs in state IDLE to go to READY and prepare for anticollision or selection. 7 bit frame.
	PICC_CMD_WUPA = 0x52,// Wake-UP command, Type A. Invites PICCs in state IDLE and HALT to go to READY(*) and prepare for anticollision or selection. 7 bit frame.
	PICC_CMD_CT = 0x88,	// Cascade Tag. Not really a command, but used during anti collision.
	PICC_CMD_SEL_CL1 = 0x93,		// Anti collision/Select, Cascade Level 1
	PICC_CMD_SEL_CL2 = 0x95,		// Anti collision/Select, Cascade Level 2
	PICC_CMD_SEL_CL3 = 0x97,		// Anti collision/Select, Cascade Level 3
	PICC_CMD_HLTA = 0x50,// HaLT command, Type A. Instructs an ACTIVE PICC to go to state HALT.
	PICC_CMD_RATS = 0xE0,     // Request command for Answer To Reset.
	// The commands used for MIFARE Classic (from http://www.mouser.com/ds/2/302/MF1S503x-89574.pdf, Section 9)
	// Use PCD_MFAuthent to authenticate access to a sector, then use these commands to read/write/modify the blocks on the sector.
	// The read/write commands can also be used for MIFARE Ultralight.
	PICC_CMD_MF_AUTH_KEY_A = 0x60,		// Perform authentication with Key A
	PICC_CMD_MF_AUTH_KEY_B = 0x61,		// Perform authentication with Key B
	PICC_CMD_MF_READ = 0x30,// Reads one 16 uint8_t block from the authenticated sector of the PICC. Also used for MIFARE Ultralight.
	PICC_CMD_MF_WRITE = 0xA0,// Writes one 16 uint8_t block to the authenticated sector of the PICC. Called "COMPATIBILITY WRITE" for MIFARE Ultralight.
	PICC_CMD_MF_DECREMENT = 0xC0,// Decrements the contents of a block and stores the result in the internal data register.
	PICC_CMD_MF_INCREMENT = 0xC1,// Increments the contents of a block and stores the result in the internal data register.
	PICC_CMD_MF_RESTORE = 0xC2,	// Reads the contents of a block into the internal data register.
	PICC_CMD_MF_TRANSFER = 0xB0,// Writes the contents of the internal data register to a block.
	// The commands used for MIFARE Ultralight (from http://www.nxp.com/documents/data_sheet/MF0ICU1.pdf, Section 8.6)
	// The PICC_CMD_MF_READ and PICC_CMD_MF_WRITE can also be used for MIFARE Ultralight.
	PICC_CMD_UL_WRITE = 0xA2		// Writes one 4 uint8_t page to the PICC.
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
rc52x_result_t PCD_CalculateCRC(rc52x_t *rc52x, uint8_t *data, uint8_t length,
		uint8_t *result);

/////////////////////////////////////////////////////////////////////////////////////
// Functions for manipulating the MFRC522
/////////////////////////////////////////////////////////////////////////////////////
void PCD_Init(rc52x_t *rc52x);
void PCD_Reset(rc52x_t *rc52x);
void PCD_AntennaOn(rc52x_t *rc52x);
void PCD_AntennaOff(rc52x_t *rc52x);
uint8_t PCD_GetAntennaGain(rc52x_t *rc52x);
void PCD_SetAntennaGain(rc52x_t *rc52x, uint8_t mask);
bool PCD_PerformSelfTest(rc52x_t *rc52x);

/////////////////////////////////////////////////////////////////////////////////////
// Power control functions
/////////////////////////////////////////////////////////////////////////////////////
void PCD_SoftPowerDown(rc52x_t *rc52x);
void PCD_SoftPowerUp(rc52x_t *rc52x);

/////////////////////////////////////////////////////////////////////////////////////
// Functions for communicating with PICCs
/////////////////////////////////////////////////////////////////////////////////////
rc52x_result_t PCD_TransceiveData(rc52x_t *rc52x, uint8_t *sendData,
		uint8_t sendLen, uint8_t *backData, uint8_t *backLen,
		uint8_t *validBits, uint8_t rxAlign, bool checkCRC);
rc52x_result_t PCD_CommunicateWithPICC(rc52x_t *rc52x, uint8_t command,
		uint8_t waitIRq, uint8_t *sendData, uint8_t sendLen, uint8_t *backData,
		uint8_t *backLen, uint8_t *validBits, uint8_t rxAlign, bool checkCRC);
rc52x_result_t PICC_RequestA(rc52x_t *rc52x, uint8_t *bufferATQA,
		uint8_t *bufferSize);
rc52x_result_t PICC_WakeupA(rc52x_t *rc52x, uint8_t *bufferATQA,
		uint8_t *bufferSize);
rc52x_result_t PICC_REQA_or_WUPA(rc52x_t *rc52x, uint8_t command,
		uint8_t *bufferATQA, uint8_t *bufferSize);
rc52x_result_t PICC_Select(rc52x_t *rc52x, Uid *uid, uint8_t validBits);
rc52x_result_t PICC_HaltA(rc52x_t *rc52x);

/////////////////////////////////////////////////////////////////////////////////////
// Functions for communicating with MIFARE PICCs
/////////////////////////////////////////////////////////////////////////////////////
rc52x_result_t PCD_Authenticate(rc52x_t *rc52x, uint8_t command,
		uint8_t blockAddr, MIFARE_Key *key, Uid *uid);
void PCD_StopCrypto1(rc52x_t *rc52x);
rc52x_result_t MIFARE_Read(rc52x_t *rc52x, uint8_t blockAddr, uint8_t *buffer,
		uint8_t *bufferSize);
rc52x_result_t MIFARE_Write(rc52x_t *rc52x, uint8_t blockAddr, uint8_t *buffer,
		uint8_t bufferSize);
rc52x_result_t MIFARE_Ultralight_Write(rc52x_t *rc52x, uint8_t page,
		uint8_t *buffer, uint8_t bufferSize);
rc52x_result_t MIFARE_Decrement(rc52x_t *rc52x, uint8_t blockAddr,
		int32_t delta);
rc52x_result_t MIFARE_Increment(rc52x_t *rc52x, uint8_t blockAddr,
		int32_t delta);
rc52x_result_t MIFARE_Restore(rc52x_t *rc52x, uint8_t blockAddr);
rc52x_result_t MIFARE_Transfer(rc52x_t *rc52x, uint8_t blockAddr);
rc52x_result_t MIFARE_GetValue(rc52x_t *rc52x, uint8_t blockAddr,
		int32_t *value);
rc52x_result_t MIFARE_SetValue(rc52x_t *rc52x, uint8_t blockAddr,
		int32_t value);
rc52x_result_t PCD_NTAG216_AUTH(rc52x_t *rc52x, uint8_t *passWord,
		uint8_t pACK[]);

/////////////////////////////////////////////////////////////////////////////////////
// Support functions
/////////////////////////////////////////////////////////////////////////////////////
rc52x_result_t PCD_MIFARE_Transceive(rc52x_t *rc52x, uint8_t *sendData,
		uint8_t sendLen, bool acceptTimeout);

// Advanced functions for MIFARE
void MIFARE_SetAccessBits(rc52x_t *rc52x, uint8_t *accessBitBuffer, uint8_t g0,
		uint8_t g1, uint8_t g2, uint8_t g3);
bool MIFARE_OpenUidBackdoor(rc52x_t *rc52x, bool logErrors);
bool MIFARE_SetUid(rc52x_t *rc52x, uint8_t *newUid, uint8_t uidSize,
		bool logErrors);
bool MIFARE_UnbrickUidSector(rc52x_t *rc52x, bool logErrors);

#endif
