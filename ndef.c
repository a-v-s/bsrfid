/*
 * ndef.c
 *
 *  Created on: 31 aug. 2022
 *      Author: andre
 */

#include "ndef.h"

int ndef_record_parse(void *data, size_t size) {
	// Note: Proof of Concept implementation
	// This lacks proper bounds checking

	int offset = 0;
	while (offset < size) {
		ndef_record_header_t *header = (ndef_record_header_t*) (data + offset);
		if (header->sr) {
			// short record
			offset++;
			size_t type_length = *(uint8_t*) (data + offset);
			offset++;
			size_t payload_length = *(uint8_t*) (data + offset);
			offset++;
			size_t id_length = 0;
			if (header->il) {
				id_length = *(uint8_t*) (data + offset);
				offset++;
			}

			{

				uint8_t type[type_length];
				//uint8_t type[64];
				memcpy(type, data + offset, type_length);
				offset += type_length;

				uint8_t id[id_length];
				//uint8_t id[64];
				memcpy(id, data + offset, id_length);
				offset += id_length;

				uint8_t payload[payload_length];
				//uint8_t payload[1024];
				memcpy(payload, data + offset, payload_length);
				offset += payload_length;

			}

		} else {
			// long record
			offset++;
			size_t type_length = (ndef_record_header_t*) (data + offset);
		}
	}
}

int ndef_message_parse(void *data, size_t size) {

}

int ndef_tlv_parse(void *data, size_t size) {
	int offset = 0;
	while (offset < size) {
		int t = *(uint8_t*) (data + offset);
		if (t == 0xFE) {
			// End marker
			return 0;
		}

		offset++;
		size_t s = *(uint8_t*) (data + offset);
		offset++;
		if (s == 0xFF) {
			s = *(uint8_t*) (data + offset);
			s <<= 8;
			offset++;
			s |= *(uint8_t*) (data + offset);
			offset++;
		}
		ndef_record_parse(data + offset, s);
		offset += s;
	}

}
