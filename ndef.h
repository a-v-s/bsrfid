/*
 * ndef.h
 *
 *  Created on: 31 aug. 2022
 *      Author: andre
 */

#ifndef BSRFID_NDEF_H_
#define BSRFID_NDEF_H_


#include <stdlib.h>
#include <stdint.h>

// T1T http://apps4android.org/nfc-specifications/NFCForum-TS-Type-1-Tag_1.1.pdf
// T2T https://apps4android.org/nfc-specifications/NFCForum-TS-Type-2-Tag_1.1.pdf
// T3T http://apps4android.org/nfc-specifications/NFCForum-TS-Type-3-Tag_1.1.pdf
// T3T https://e2e.ti.com/cfs-file/__key/communityserver-discussions-components-files/667/NFC-Forum-Type-3-Tag-Platform-Operations-with-TRF7970A_5F00_10_5F00_08_5F00_2014.pdf
// T4T http://apps4android.org/nfc-specifications/NFCForum-TS-Type-4-Tag_2.0.pdf
// T5T https://www.st.com/resource/en/application_note/an5151-ndef-management-with-st25dv02kw1-and-st25dv02kw2-stmicroelectronics.pdf

#pragma pack(push,1)

// NFC CC
// Type 1: (size+1) * 8
// Type 2: size * 8


typedef struct {
	uint8_t magic;
	unsigned int version_minor : 4;
	unsigned int version_major : 4;
	uint8_t size;
	uint8_t rw;
} nfc_cc_t;

// NFC CC
// Type 5: Smart Phone: size * 8
//         NFC:         (size+1) * 8

typedef struct {
	uint8_t magic;
	unsigned int write: 2;
	unsigned int read: 2;
	unsigned int version_minor : 2;
	unsigned int version_major : 2;
	uint8_t size;
	uint8_t feature;
} nfc_cc_t5_t;

typedef struct {
	unsigned int tnf:3;
	unsigned int il : 1;
	unsigned int sr : 1;
	unsigned int cf : 1;
	unsigned int me : 1;
	unsigned int mb : 1;
} ndef_record_header_t;




#define NFC_CC_MAGIC (0xE1)

typedef struct {
	unsigned int version_minor : 2;
	unsigned int version_major : 2;
	uint8_t nbr;
	uint8_t nbw;
	// TODO
} nfc_ab_t;

// NFC AB
// Type 3

#pragma pack(pop)

#endif /* BSRFID_NDEF_H_ */
