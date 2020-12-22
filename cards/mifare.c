/*
 * mifare.c
 *
 *  Created on: 20 dec. 2020
 *      Author: andre
 */




/**
 * Reads 16 uint8_ts (+ 2 uint8_ts CRC_A) from the active PICC.
 *
 * For MIFARE Classic the sector containing the block must be authenticated before calling this function.
 *
 * For MIFARE Ultralight only addresses 00h to 0Fh are decoded.
 * The MF0ICU1 returns a NAK for higher addresses.
 * The MF0ICU1 responds to the READ command by sending 16 uint8_ts starting from the page address defined by the command argument.
 * For example; if blockAddr is 03h then pages 03h, 04h, 05h, 06h are returned.
 * A roll-back is implemented: If blockAddr is 0Eh, then the contents of pages 0Eh, 0Fh, 00h and 01h are returned.
 *
 * The buffer must be at least 18 uint8_ts because a CRC_A is also returned.
 * Checks the CRC_A before returning STATUS_OK.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t MIFARE_Read(rc52x_t *rc52x, uint8_t blockAddr, ///< MIFARE Classic: The block (0-0xff) number. MIFARE Ultralight: The first page to return data from.
		uint8_t *buffer,		///< The buffer to store the data in
		uint8_t *bufferSize	///< Buffer size, at least 18 uint8_ts. Also number of uint8_ts returned if STATUS_OK.
		) {
	rc52x_result_t result;

	// Sanity check
	if (buffer == nullptr || *bufferSize < 18) {
		return STATUS_NO_ROOM;
	}

	// Build command buffer
	buffer[0] = PICC_CMD_MF_READ;
	buffer[1] = blockAddr;
	// Calculate CRC_A
	result = PCD_CalculateCRC(buffer, 2, &buffer[2]);
	if (result != STATUS_OK) {
		return result;
	}

	// Transmit the buffer and receive the response, validate CRC_A.
	return PCD_TransceiveData(buffer, 4, buffer, bufferSize, nullptr, 0, true);
} // End MIFARE_Read()

/**
 * Writes 16 uint8_ts to the active PICC.
 *
 * For MIFARE Classic the sector containing the block must be authenticated before calling this function.
 *
 * For MIFARE Ultralight the operation is called "COMPATIBILITY WRITE".
 * Even though 16 uint8_ts are transferred to the Ultralight PICC, only the least significant 4 uint8_ts (uint8_ts 0 to 3)
 * are written to the specified address. It is recommended to set the remaining uint8_ts 04h to 0Fh to all logic 0.
 * *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t MIFARE_Write(rc52x_t *rc52x, uint8_t blockAddr, ///< MIFARE Classic: The block (0-0xff) number. MIFARE Ultralight: The page (2-15) to write to.
		uint8_t *buffer,	///< The 16 uint8_ts to write to the PICC
		uint8_t bufferSize///< Buffer size, must be at least 16 uint8_ts. Exactly 16 uint8_ts are written.
		) {
	rc52x_result_t result;

	// Sanity check
	if (buffer == nullptr || bufferSize < 16) {
		return STATUS_INVALID;
	}

	// Mifare Classic protocol requires two communications to perform a write.
	// Step 1: Tell the PICC we want to write to block blockAddr.
	uint8_t cmdBuffer[2];
	cmdBuffer[0] = PICC_CMD_MF_WRITE;
	cmdBuffer[1] = blockAddr;
	result = PCD_MIFARE_Transceive(cmdBuffer, 2); // Adds CRC_A and checks that the response is MF_ACK.
	if (result != STATUS_OK) {
		return result;
	}

	// Step 2: Transfer the data
	result = PCD_MIFARE_Transceive(buffer, bufferSize); // Adds CRC_A and checks that the response is MF_ACK.
	if (result != STATUS_OK) {
		return result;
	}

	return STATUS_OK;
} // End MIFARE_Write()

/**
 * Writes a 4 uint8_t page to the active MIFARE Ultralight PICC.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t MIFARE_Ultralight_Write(rc52x_t *rc52x, uint8_t page, ///< The page (2-15) to write to.
		uint8_t *buffer,	///< The 4 uint8_ts to write to the PICC
		uint8_t bufferSize///< Buffer size, must be at least 4 uint8_ts. Exactly 4 uint8_ts are written.
		) {
	rc52x_result_t result;

	// Sanity check
	if (buffer == nullptr || bufferSize < 4) {
		return STATUS_INVALID;
	}

	// Build commmand buffer
	uint8_t cmdBuffer[6];
	cmdBuffer[0] = PICC_CMD_UL_WRITE;
	cmdBuffer[1] = page;
	memcpy(&cmdBuffer[2], buffer, 4);

	// Perform the write
	result = PCD_MIFARE_Transceive(cmdBuffer, 6); // Adds CRC_A and checks that the response is MF_ACK.
	if (result != STATUS_OK) {
		return result;
	}
	return STATUS_OK;
} // End MIFARE_Ultralight_Write()

/**
 * MIFARE Decrement subtracts the delta from the value of the addressed block, and stores the result in a volatile memory.
 * For MIFARE Classic only. The sector containing the block must be authenticated before calling this function.
 * Only for blocks in "value block" mode, ie with access bits [C1 C2 C3] = [110] or [001].
 * Use MIFARE_Transfer() to store the result in a block.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t MIFARE_Decrement(rc52x_t *rc52x, uint8_t blockAddr, ///< The block (0-0xff) number.
		int32_t delta ///< This number is subtracted from the value of block blockAddr.
		) {
	return MIFARE_TwoStepHelper(PICC_CMD_MF_DECREMENT, blockAddr, delta);
} // End MIFARE_Decrement()

/**
 * MIFARE Increment adds the delta to the value of the addressed block, and stores the result in a volatile memory.
 * For MIFARE Classic only. The sector containing the block must be authenticated before calling this function.
 * Only for blocks in "value block" mode, ie with access bits [C1 C2 C3] = [110] or [001].
 * Use MIFARE_Transfer() to store the result in a block.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t MIFARE_Increment(rc52x_t *rc52x, uint8_t blockAddr, ///< The block (0-0xff) number.
		int32_t delta ///< This number is added to the value of block blockAddr.
		) {
	return MIFARE_TwoStepHelper(PICC_CMD_MF_INCREMENT, blockAddr, delta);
} // End MIFARE_Increment()

/**
 * MIFARE Restore copies the value of the addressed block into a volatile memory.
 * For MIFARE Classic only. The sector containing the block must be authenticated before calling this function.
 * Only for blocks in "value block" mode, ie with access bits [C1 C2 C3] = [110] or [001].
 * Use MIFARE_Transfer() to store the result in a block.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t MIFARE_Restore(rc52x_t *rc52x, uint8_t blockAddr ///< The block (0-0xff) number.
		) {
	// The datasheet describes Restore as a two step operation, but does not explain what data to transfer in step 2.
	// Doing only a single step does not work, so I chose to transfer 0L in step two.
	return MIFARE_TwoStepHelper(PICC_CMD_MF_RESTORE, blockAddr, 0L);
} // End MIFARE_Restore()

/**
 * Helper function for the two-step MIFARE Classic protocol operations Decrement, Increment and Restore.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t MIFARE_TwoStepHelper(rc52x_t *rc52x, uint8_t command,///< The command to use
		uint8_t blockAddr,	///< The block (0-0xff) number.
		int32_t data		///< The data to transfer in step 2
		) {
	rc52x_result_t result;
	uint8_t cmdBuffer[2]; // We only need room for 2 uint8_ts.

	// Step 1: Tell the PICC the command and block address
	cmdBuffer[0] = command;
	cmdBuffer[1] = blockAddr;
	result = PCD_MIFARE_Transceive(cmdBuffer, 2); // Adds CRC_A and checks that the response is MF_ACK.
	if (result != STATUS_OK) {
		return result;
	}

	// Step 2: Transfer the data
	result = PCD_MIFARE_Transceive((uint8_t*) &data, 4, true); // Adds CRC_A and accept timeout as success.
	if (result != STATUS_OK) {
		return result;
	}

	return STATUS_OK;
} // End MIFARE_TwoStepHelper()

/**
 * MIFARE Transfer writes the value stored in the volatile memory into one MIFARE Classic block.
 * For MIFARE Classic only. The sector containing the block must be authenticated before calling this function.
 * Only for blocks in "value block" mode, ie with access bits [C1 C2 C3] = [110] or [001].
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t MIFARE_Transfer(rc52x_t *rc52x, uint8_t blockAddr ///< The block (0-0xff) number.
		) {
	rc52x_result_t result;
	uint8_t cmdBuffer[2]; // We only need room for 2 uint8_ts.

	// Tell the PICC we want to transfer the result into block blockAddr.
	cmdBuffer[0] = PICC_CMD_MF_TRANSFER;
	cmdBuffer[1] = blockAddr;
	result = PCD_MIFARE_Transceive(cmdBuffer, 2); // Adds CRC_A and checks that the response is MF_ACK.
	if (result != STATUS_OK) {
		return result;
	}
	return STATUS_OK;
} // End MIFARE_Transfer()

/**
 * Helper routine to read the current value from a Value Block.
 *
 * Only for MIFARE Classic and only for blocks in "value block" mode, that
 * is: with access bits [C1 C2 C3] = [110] or [001]. The sector containing
 * the block must be authenticated before calling this function.
 *
 * @param[in]   blockAddr   The block (0x00-0xff) number.
 * @param[out]  value       Current value of the Value Block.
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t MIFARE_GetValue(rc52x_t *rc52x, uint8_t blockAddr, int32_t *value) {
	rc52x_result_t status;
	uint8_t buffer[18];
	uint8_t size = sizeof(buffer);

	// Read the block
	status = MIFARE_Read(blockAddr, buffer, &size);
	if (status == STATUS_OK) {
		// Extract the value
		*value = (int32_t(buffer[3]) << 24) | (int32_t(buffer[2]) << 16)
				| (int32_t(buffer[1]) << 8) | int32_t(buffer[0]);
	}
	return status;
} // End MIFARE_GetValue()

/**
 * Helper routine to write a specific value into a Value Block.
 *
 * Only for MIFARE Classic and only for blocks in "value block" mode, that
 * is: with access bits [C1 C2 C3] = [110] or [001]. The sector containing
 * the block must be authenticated before calling this function.
 *
 * @param[in]   blockAddr   The block (0x00-0xff) number.
 * @param[in]   value       New value of the Value Block.
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t MIFARE_SetValue(rc52x_t *rc52x, uint8_t blockAddr, int32_t value) {
	uint8_t buffer[18];

	// Translate the int32_t into 4 uint8_ts; repeated 2x in value block
	buffer[0] = buffer[8] = (value & 0xFF);
	buffer[1] = buffer[9] = (value & 0xFF00) >> 8;
	buffer[2] = buffer[10] = (value & 0xFF0000) >> 16;
	buffer[3] = buffer[11] = (value & 0xFF000000) >> 24;
	// Inverse 4 uint8_ts also found in value block
	buffer[4] = ~buffer[0];
	buffer[5] = ~buffer[1];
	buffer[6] = ~buffer[2];
	buffer[7] = ~buffer[3];
	// Address 2x with inverse address 2x
	buffer[12] = buffer[14] = blockAddr;
	buffer[13] = buffer[15] = ~blockAddr;

	// Write the whole data block
	return MIFARE_Write(blockAddr, buffer, 16);
} // End MIFARE_SetValue()


/**
 * Calculates the bit pattern needed for the specified access bits. In the [C1 C2 C3] tuples C1 is MSB (=4) and C3 is LSB (=1).
 */
void MIFARE_SetAccessBits(rc52x_t *rc52x, uint8_t *accessBitBuffer,	///< Pointer to uint8_t 6, 7 and 8 in the sector trailer. uint8_ts [0..2] will be set.
		uint8_t g0,	///< Access bits [C1 C2 C3] for block 0 (for sectors 0-31) or blocks 0-4 (for sectors 32-39)
		uint8_t g1,	///< Access bits C1 C2 C3] for block 1 (for sectors 0-31) or blocks 5-9 (for sectors 32-39)
		uint8_t g2,	///< Access bits C1 C2 C3] for block 2 (for sectors 0-31) or blocks 10-14 (for sectors 32-39)
		uint8_t g3///< Access bits C1 C2 C3] for the sector trailer, block 3 (for sectors 0-31) or block 15 (for sectors 32-39)
		) {
	uint8_t c1 = ((g3 & 4) << 1) | ((g2 & 4) << 0) | ((g1 & 4) >> 1)
			| ((g0 & 4) >> 2);
	uint8_t c2 = ((g3 & 2) << 2) | ((g2 & 2) << 1) | ((g1 & 2) << 0)
			| ((g0 & 2) >> 1);
	uint8_t c3 = ((g3 & 1) << 3) | ((g2 & 1) << 2) | ((g1 & 1) << 1)
			| ((g0 & 1) << 0);

	accessBitBuffer[0] = (~c2 & 0xF) << 4 | (~c1 & 0xF);
	accessBitBuffer[1] = c1 << 4 | (~c3 & 0xF);
	accessBitBuffer[2] = c3 << 4 | c2;
} // End MIFARE_SetAccessBits()

/**
 * Performs the "magic sequence" needed to get Chinese UID changeable
 * Mifare cards to allow writing to sector 0, where the card UID is stored.
 *
 * Note that you do not need to have selected the card through REQA or WUPA,
 * this sequence works immediately when the card is in the reader vicinity.
 * This means you can use this method even on "bricked" cards that your reader does
 * not recognise anymore (see MIFARE_UnbrickUidSector).
 *
 * Of course with non-bricked devices, you're free to select them before calling this function.
 */
bool MIFARE_OpenUidBackdoor(rc52x_t *rc52x, bool logErrors) {
	// Magic sequence:
	// > 50 00 57 CD (HALT + CRC)
	// > 40 (7 bits only)
	// < A (4 bits only)
	// > 43
	// < A (4 bits only)
	// Then you can write to sector 0 without authenticating

	PICC_HaltA(); // 50 00 57 CD

	uint8_t cmd = 0x40;
	uint8_t validBits = 7; /* Our command is only 7 bits. After receiving card response,
	 this will contain amount of valid response bits. */
	uint8_t response[32]; // Card's response is written here
	uint8_t received;
	rc52x_result_t status = PCD_TransceiveData(&cmd, (uint8_t) 1, response,
			&received, &validBits, (uint8_t) 0, false); // 40
	if (status != STATUS_OK) {
		if (logErrors) {
			Serial.println(
					F(
							"Card did not respond to 0x40 after HALT command. Are you sure it is a UID changeable one?"));
			Serial.print(F("Error name: "));
			Serial.println(Getrc52x_result_tName(status));
		}
		return false;
	}
	if (received != 1 || response[0] != 0x0A) {
		if (logErrors) {
			Serial.print(F("Got bad response on backdoor 0x40 command: "));
			Serial.print(response[0], HEX);
			Serial.print(F(" ("));
			Serial.print(validBits);
			Serial.print(F(" valid bits)\r\n"));
		}
		return false;
	}

	cmd = 0x43;
	validBits = 8;
	status = PCD_TransceiveData(&cmd, (uint8_t) 1, response, &received,
			&validBits, (uint8_t) 0, false); // 43
	if (status != STATUS_OK) {
		if (logErrors) {
			Serial.println(
					F(
							"Error in communication at command 0x43, after successfully executing 0x40"));
			Serial.print(F("Error name: "));
			Serial.println(Getrc52x_result_tName(status));
		}
		return false;
	}
	if (received != 1 || response[0] != 0x0A) {
		if (logErrors) {
			Serial.print(F("Got bad response on backdoor 0x43 command: "));
			Serial.print(response[0], HEX);
			Serial.print(F(" ("));
			Serial.print(validBits);
			Serial.print(F(" valid bits)\r\n"));
		}
		return false;
	}

	// You can now write to sector 0 without authenticating!
	return true;
} // End MIFARE_OpenUidBackdoor()

/**
 * Reads entire block 0, including all manufacturer data, and overwrites
 * that block with the new UID, a freshly calculated BCC, and the original
 * manufacturer data.
 *
 * It assumes a default KEY A of 0xFFFFFFFFFFFF.
 * Make sure to have selected the card before this function is called.
 */
bool MIFARE_SetUid(uint8_t *newUid, uint8_t uidSize, bool logErrors) {

	// UID + BCC uint8_t can not be larger than 16 together
	if (!newUid || !uidSize || uidSize > 15) {
		if (logErrors) {
			Serial.println(
					F("New UID buffer empty, size 0, or size > 15 given"));
		}
		return false;
	}

	// Authenticate for reading
	MIFARE_Key key = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	rc52x_result_t status = PCD_Authenticate(PICC_CMD_MF_AUTH_KEY_A, (uint8_t) 1,
			&key, &uid);
	if (status != STATUS_OK) {

		if (status == STATUS_TIMEOUT) {
			// We get a read timeout if no card is selected yet, so let's select one

			// Wake the card up again if sleeping
//			  uint8_t atqa_answer[2];
//			  uint8_t atqa_size = 2;
//			  PICC_WakeupA(atqa_answer, &atqa_size);

			if (!PICC_IsNewCardPresent() || !PICC_ReadCardSerial()) {
				Serial.println(
						F(
								"No card was previously selected, and none are available. Failed to set UID."));
				return false;
			}

			status = PCD_Authenticate(PICC_CMD_MF_AUTH_KEY_A, (uint8_t) 1, &key,
					&uid);
			if (status != STATUS_OK) {
				// We tried, time to give up
				if (logErrors) {
					Serial.println(
							F(
									"Failed to authenticate to card for reading, could not set UID: "));
					Serial.println(Getrc52x_result_tName(status));
				}
				return false;
			}
		} else {
			if (logErrors) {
				Serial.print(F("PCD_Authenticate() failed: "));
				Serial.println(Getrc52x_result_tName(status));
			}
			return false;
		}
	}

	// Read block 0
	uint8_t block0_buffer[18];
	uint8_t uint8_tCount = sizeof(block0_buffer);
	status = MIFARE_Read((uint8_t) 0, block0_buffer, &uint8_tCount);
	if (status != STATUS_OK) {
		if (logErrors) {
			Serial.print(F("MIFARE_Read() failed: "));
			Serial.println(Getrc52x_result_tName(status));
			Serial.println(
					F(
							"Are you sure your KEY A for sector 0 is 0xFFFFFFFFFFFF?"));
		}
		return false;
	}

	// Write new UID to the data we just read, and calculate BCC uint8_t
	uint8_t bcc = 0;
	for (uint8_t i = 0; i < uidSize; i++) {
		block0_buffer[i] = newUid[i];
		bcc ^= newUid[i];
	}

	// Write BCC uint8_t to buffer
	block0_buffer[uidSize] = bcc;

	// Stop encrypted traffic so we can send raw uint8_ts
	PCD_StopCrypto1();

	// Activate UID backdoor
	if (!MIFARE_OpenUidBackdoor(logErrors)) {
		if (logErrors) {
			Serial.println(F("Activating the UID backdoor failed."));
		}
		return false;
	}

	// Write modified block 0 back to card
	status = MIFARE_Write((uint8_t) 0, block0_buffer, (uint8_t) 16);
	if (status != STATUS_OK) {
		if (logErrors) {
			Serial.print(F("MIFARE_Write() failed: "));
			Serial.println(Getrc52x_result_tName(status));
		}
		return false;
	}

	// Wake the card up again
	uint8_t atqa_answer[2];
	uint8_t atqa_size = 2;
	PICC_WakeupA(atqa_answer, &atqa_size);

	return true;
}

/**
 * Resets entire sector 0 to zeroes, so the card can be read again by readers.
 */
bool MIFARE_UnbrickUidSector(bool logErrors) {
	MIFARE_OpenUidBackdoor(logErrors);

	uint8_t block0_buffer[] = { 0x01, 0x02, 0x03, 0x04, 0x04, 0x08, 0x04, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	// Write modified block 0 back to card
	rc52x_result_t status = MIFARE_Write((uint8_t) 0, block0_buffer, (uint8_t) 16);
	if (status != STATUS_OK) {
		if (logErrors) {
			Serial.print(F("MIFARE_Write() failed: "));
			Serial.println(Getrc52x_result_tName(status));
		}
		return false;
	}
	return true;
}
