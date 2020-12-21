/*
 * iso14443a.h
 *
 *  Created on: 30 nov. 2020
 *      Author: andre
 */

#ifndef BSRFID_CARDS_ISO14443A_H_
#define BSRFID_CARDS_ISO14443A_H_

#pragma pack (push,1)

typedef enum {
		PICC_CMD_REQA			= 0x26,		// REQuest command, Type A. Invites PICCs in state IDLE to go to READY and prepare for anticollision or selection. 7 bit frame.
		PICC_CMD_WUPA			= 0x52,		// Wake-UP command, Type A. Invites PICCs in state IDLE and HALT to go to READY(*) and prepare for anticollision or selection. 7 bit frame.
		PICC_CMD_CT				= 0x88,		// Cascade Tag. Not really a command, but used during anti collision.
		PICC_CMD_SEL_CL1		= 0x93,		// Anti collision/Select, Cascade Level 1
		PICC_CMD_SEL_CL2		= 0x95,		// Anti collision/Select, Cascade Level 2
		PICC_CMD_SEL_CL3		= 0x97,		// Anti collision/Select, Cascade Level 3
		PICC_CMD_HLTA			= 0x50,		// HaLT command, Type A. Instructs an ACTIVE PICC to go to state HALT.
		PICC_CMD_RATS           = 0xE0,     // Request command for Answer To Reset.
} iso14443a_command_t;

typedef enum {
	iso14443a_atqa_uid_size_single = 0b00,
	iso14443a_atqa_uid_size_double = 0b01,
	iso14443a_atqa_uid_size_triple = 0b10,
	iso14443a_atqa_uid_size_rfu    = 0b11,
} iso14443a_atqa_uid_size_t;

typedef struct {
	union {
		struct {
			// Found in documentation, (NXP AN10833)
			unsigned int bit_frame      : 5;
			unsigned int rfu0			: 1;
			iso14443a_atqa_uid_size_t uid_size		: 2;
			unsigned int proprietary	: 4;
			unsigned int rfu1			: 4;
		};
		uint16_t as_uint16;
		uint8_t as_uint8[2];
	};
} iso14443a_atqa_t;

typedef struct {
	union {
		// N.B. As found in documentation, (eg. NXP AN10833, AN10834)
		// the bits start counting at bit1 in stead of bit0.
		// This document only discusses the bits 3, 6 and 7.
		// The other bits are not discussed, so whether they
		// are defined in ISO/IEC 14443-3 remains unknown
		// as ISO/IEC 14443-3 is behind a paywall.
		unsigned int bit1 : 1;
		unsigned int bit2 : 1;
		unsigned int uid_not_complete : 1;
		unsigned int bit4 : 1;
		unsigned int bit5 : 1;
		unsigned int iso14443_4_compliant : 1;
		unsigned int iso18092_compliant : 1;
		unsigned int bit8 : 1;
	};
	uint8_t as_uint8;
} iso14443a_sak_t;
#pragma pack (pop)
#endif /* BSRFID_CARDS_ISO14443A_H_ */
