/*
 * picc.c
 *
 *  Created on: 29 dec. 2020
 *      Author: andre
 */

#include "picc.h"
#include "pdc.h"

/**
 * Transmits a REQuest command, Type A. Invites PICCs in state IDLE to go to READY and prepare for anticollision or selection. 7 bit frame.
 * Beware: When two PICCs are in the field at the same time I often get STATUS_TIMEOUT - probably due do bad antenna design.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t PICC_RequestA(bs_pdc_t *pdc, picc_t *picc) {
	picc->protocol = picc_protocol_iso14443a;
	size_t size = sizeof(iso14443a_atqa_t);
	return PICC_REQA_or_WUPA(pdc, PICC_CMD_REQA, &picc->atqa, &size);
} // End PICC_RequestA()

/**
 * Transmits a Wake-UP command, Type A. Invites PICCs in state IDLE and HALT to go to READY(*) and prepare for anticollision or selection. 7 bit frame.
 * Beware: When two PICCs are in the field at the same time I often get STATUS_TIMEOUT - probably due do bad antenna design.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t PICC_WakeupA(bs_pdc_t *pdc, picc_t *picc) {
	if (picc->protocol != picc_protocol_iso14443a)
		return STATUS_INVALID;
	size_t size = sizeof(iso14443a_atqa_t);
	return PICC_REQA_or_WUPA(pdc, PICC_CMD_WUPA, &picc->atqa, &size);
} //  // End PICC_WakeupA()

/**
 * Transmits REQA or WUPA commands.
 * Beware: When two PICCs are in the field at the same time I often get STATUS_TIMEOUT - probably due do bad antenna design.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t PICC_REQA_or_WUPA(bs_pdc_t *pdc, uint8_t command, ///< The command to send - PICC_CMD_REQA or PICC_CMD_WUPA
		uint8_t *bufferATQA, ///< The buffer to store the ATQA (Answer to request) in
		size_t *bufferSize///< Buffer size, at least two bytes. Also number of bytes returned if STATUS_OK.
		) {
	uint8_t validBits;
	rc52x_result_t status;

	if (bufferATQA == NULL || *bufferSize < 2) {// The ATQA response is 2 bytes long.
		return STATUS_NO_ROOM;
	}

	// Do we need to keep this into the port?
	//RC52X_ClearRegisterBitMask(rc52x, RC52X_REG_CollReg, 0x80);// ValuesAfterColl=1 => Bits received after collision are cleared.

	validBits = 7;// For REQA and WUPA we need the short frame format - transmit only 7 bits of the last (and only) uint8_t. TxLastBits = BitFramingReg[2..0]


	status = pdc->TransceiveData(pdc, &command, 1, bufferATQA, bufferSize,
			&validBits, 0, NULL, false, false);
	//status = RC52X_TransceiveData(rc52x, &command, 1, bufferATQA, bufferSize, &validBits, 0, false);
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
 * A PICC UID consists of 4, 7 or 10 bytes.
 * Only 4 uint8_ts can be specified in a SELECT command, so for the longer UIDs two or three iterations are used:
 * 		UID size	Number of UID uint8_ts		Cascade levels		Example of PICC
 * 		========	===================		==============		===============
 * 		single				 4						1				MIFARE Classic
 * 		double				 7						2				MIFARE Ultralight
 * 		triple				10						3				Not currently in use?
 *
 *
 * uid:  Pointer to Uid struct. Normally output, but can also be used to supply a known UID.
 * validBits: The number of known UID bits supplied in *uid. Normally 0. If set you must also supply uid->size.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t PICC_Select(bs_pdc_t *pdc, picc_t *picc, uint8_t validBits) {
	bool uidComplete;
	bool selectDone;
	bool useCascadeTag;
	uint8_t cascadeLevel = 1;
	rc52x_result_t result;
	uint8_t count;
	uint8_t checkBit;
	uint8_t index;
	uint8_t uidIndex; // The first index in uid->uidByte[] that is used in the current Cascade Level.
	int8_t currentLevelKnownBits; // The number of known UID bits in the current Cascade Level.
	uint8_t buffer[9]; // The SELECT/ANTICOLLISION commands uses a 7 uint8_t standard frame + 2 uint8_ts CRC_A
	uint8_t bufferUsed;	// The number of uint8_ts used in the buffer, ie the number of uint8_ts to transfer to the FIFO.
	uint8_t rxAlign;// Used in BitFramingReg. Defines the bit position for the first bit received.
	uint8_t txLastBits;	// Used in BitFramingReg. The number of valid bits in the last transmitted uint8_t.
	uint8_t *responseBuffer;
	size_t responseLength;
	bool sendCRC;

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
	// TODO
	//RC52X_ClearRegisterBitMask(rc52x, RC52X_REG_CollReg, 0x80);// ValuesAfterColl=1 => Bits received after collision are cleared.

	// Repeat Cascade Level loop until we have a complete UID.
	uidComplete = false;
	while (!uidComplete) {
		// Set the Cascade Level in the SEL uint8_t, find out if we need to use the Cascade Tag in uint8_t 2.
		switch (cascadeLevel) {
		case 1:
			buffer[0] = PICC_CMD_SEL_CL1;
			uidIndex = 0;
			useCascadeTag = validBits && picc->size > 4;// When we know that the UID has more than 4 uint8_ts
			break;

		case 2:
			buffer[0] = PICC_CMD_SEL_CL2;
			uidIndex = 3;
			useCascadeTag = validBits && picc->size > 7;// When we know that the UID has more than 7 uint8_ts
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
		uint8_t BytesToCopy = currentLevelKnownBits / 8
				+ (currentLevelKnownBits % 8 ? 1 : 0); // The number of uint8_ts needed to represent the known bits for this level.
		if (BytesToCopy) {
			uint8_t maxBytes = useCascadeTag ? 3 : 4; // Max 4 uint8_ts in each Cascade Level. Only 3 left if we use the Cascade Tag
			if (BytesToCopy > maxBytes) {
				BytesToCopy = maxBytes;
			}
			for (count = 0; count < BytesToCopy; count++) {
				buffer[index++] = picc->uidByte[uidIndex + count];
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

				sendCRC = true;

				txLastBits = 0; // 0 => All 8 bits are valid.
				bufferUsed = 7;
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
				sendCRC = false;
			}

			rxAlign = txLastBits;// Having a separate variable is overkill. But it makes the next line easier to read.
			uint8_t collisionPos;
			// Transmit the buffer and receive the response.
			result = pdc->TransceiveData(pdc, buffer, bufferUsed,
					responseBuffer, &responseLength, &txLastBits, rxAlign,
					&collisionPos, sendCRC, false);
			if (result == STATUS_COLLISION) {

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

		// Copy the found UID uint8_ts from buffer[] to uid->uidByte[]
		index = (buffer[2] == PICC_CMD_CT) ? 3 : 2; // source index in buffer[]
		BytesToCopy = (buffer[2] == PICC_CMD_CT) ? 3 : 4;
		for (count = 0; count < BytesToCopy; count++) {
			picc->uidByte[uidIndex + count] = buffer[index++];
		}

		// Check response SAK (Select Acknowledge)
		if (responseLength != 3 || txLastBits != 0) { // SAK must be exactly 24 bits (1 uint8_t + CRC_A).
			return STATUS_ERROR;
		}

		/*
		 // Verify CRC_A - do our own calculation and store the control in buffer[2..3] - those uint8_ts are not needed anymore.
		 result = RC52X_CalculateCRC(rc52x, responseBuffer, 1, &buffer[2]);
		 if (result != STATUS_OK) {
		 return result;
		 }

		if ((buffer[2] != responseBuffer[1])
				|| (buffer[3] != responseBuffer[2])) {
			return STATUS_CRC_WRONG;
		}
		 */

		if (responseBuffer[0] & 0x04) { // Cascade bit set - UID not complete yes
			cascadeLevel++;
		} else {
			uidComplete = true;
			picc->sak.as_uint8 = responseBuffer[0];
		}
	} // End of while (!uidComplete)

	// Set correct uid->size
	picc->size = 3 * cascadeLevel + 1;

	return STATUS_OK;
} // End PICC_Select()

/**
 * Instructs a PICC in state ACTIVE(*) to go to state HALT.
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t PICC_HaltA(bs_pdc_t *pdc) {
	rc52x_result_t result;
	uint8_t buffer[4];

	// Build command buffer
	buffer[0] = PICC_CMD_HLTA;
	buffer[1] = 0;
	// Calculate CRC_A
	result = RC52X_CalculateCRC(pdc, buffer, 2, &buffer[2]);
	if (result != STATUS_OK) {
		return result;
	}

	// Send the command.
	// The standard says:
	//		If the PICC responds with any modulation during a period of 1 ms after the end of the frame containing the
	//		HLTA command, this response shall be interpreted as 'not acknowledge'.
	// We interpret that this way: Only STATUS_TIMEOUT is a success.
	result = RC52X_TransceiveData(pdc, buffer, sizeof(buffer), NULL, 0, NULL, 0,
			false);
	if (result == STATUS_TIMEOUT) {
		return STATUS_OK;
	}
	if (result == STATUS_OK) { // That is ironically NOT ok in this case ;-)
		return STATUS_ERROR;
	}
	return result;
} // End PICC_HaltA()
