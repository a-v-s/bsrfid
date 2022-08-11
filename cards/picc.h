/*
 * rfid.h
 *
 *  Created on: 21 dec. 2020
 *      Author: andre
 */

#ifndef BSRFID_CARDS_PICC_H_
#define BSRFID_CARDS_PICC_H_

#include "iso14443a.h"
#include "iso14443b.h"
#include "iso15693.h"

#include "pdc.h"


typedef enum {
	picc_protocol_undefined,
	picc_protocol_iso14443a,    // eg. MiFare, NTAG
	picc_protocol_iso14443b,	// eg. SRI512
	picc_protocol_iso15693,     // eg. ICode
	picc_protocol_jisx_6319_4,  // eg. FeliCa
} picc_protocol_t;



	typedef struct {
		// Number of bytes in the UID.
		// For ISO 14443-A 		4, 7 or 10.
		// For ISO 14443-B		4
		// For ISO 15693		8
		// For JIS X 6319-4 	8

		picc_protocol_t protocol;
		uint8_t		size;
		uint8_t		uidByte[10];
		union {
			struct {
				iso14443a_sak_t		sak;	// The SAK (Select acknowledge) byte returned from the PICC after successful selection.
				iso14443a_atqa_t   atqa;
			};
		};
		uint8_t card_type;//TODO
		union {
		struct {
			uint8_t fixed_header;
			uint8_t vendor_id;
			uint8_t product_type;
			uint8_t product_subtype;
			uint8_t product_version_major;
			uint8_t product_version_minor;
			uint8_t storage_size;
			uint8_t protocol_type;
			uint16_t crc;
		} get_version_response;
			uint8_t rats[16];
		};
		struct {
			uint8_t page_count;
			uint8_t page_size;
			uint8_t page_offset;
			// NB... for MFC more is needed as we need to skip pages
		} memory_stucture;
	} picc_t;


	rc52x_result_t PICC_REQA_or_WUPA(bs_pdc_t *pdc, uint8_t command, ///< The command to send - PICC_CMD_REQA or PICC_CMD_WUPA
			uint8_t *bufferATQA, ///< The buffer to store the ATQA (Answer to request) in
			size_t *bufferSize	///< Buffer size, at least two bytes. Also number of bytes returned if STATUS_OK.
			) ;
#endif /* BSRFID_CARDS_PICC_H_ */
