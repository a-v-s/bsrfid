/*
 * mifare.h
 *
 *  Created on: 21 dec. 2020
 *      Author: andre
 */

#ifndef BSRFID_DRIVERS_RC522_MIFARE_H_
#define BSRFID_DRIVERS_RC522_MIFARE_H_

typedef enum {
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
	} mifare_command_t;

/////////////////////////////////////////////////////////////////////////////////////
// Functions for communicating with MIFARE PICCs
/////////////////////////////////////////////////////////////////////////////////////
rc52x_result_t PCD_Authenticate(rc52x_t *rc52x, uint8_t command,
		uint8_t blockAddr, MIFARE_Key *key, picc_t *uid);
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



#endif /* BSRFID_DRIVERS_RC522_MIFARE_H_ */
