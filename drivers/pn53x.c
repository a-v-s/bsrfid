/*
 * pn53x.c
 *
 *  Created on: 27 feb. 2022
 *      Author: andre
 */

#include <stdlib.h>
#include "pn53x.h"
#include "pn53x_transport.h"
#include "picc.h"

int pn53x_recv_ack(pn53x_t *pn53x, bool* acknack) {
	pn53x_i2c_spi_ack_nack_frame_t ack_nack = {0};
	*acknack = false;
	int result = 0;
	int retry_count = 0;
	while (!ack_nack.header.data_ready || result) {
		result =  bshal_i2cm_recv(pn53x->transport_instance.i2cm, PN53X_I2C_ADDR,&ack_nack, sizeof(pn53x_i2c_spi_ack_nack_frame_t), false);
		retry_count++;
		if (retry_count >= 25)
			return -1;
	}
	if (result) return result;
	*acknack = ack_nack.frame.ack_or_nack == PN53X_ACK;
	return result;
}



int pn53x_send_frame(pn53x_t *pn53x, char * data, int len, pn53x_i2c_spi_normal_frame_t* response) {

	pn53x_normal_frame_t request;
	request.start_code =PN53X_START_CODE;
	request.tfi = PN53X_DIR_TO_PN53X;
	request.len = len + 1;
	request.lcs = -request.len;
	int size = len + 6;
	uint8_t dcs = request.tfi;
	for (int i = 0; i < len; i++) {
		dcs += data[i];
	}
	memcpy (request.data, data, len);
	request.data[len]=(uint8_t)(-dcs);
	int result = 0;
	switch (pn53x->transport_type) {
	case bshal_transport_i2c:
		result = bshal_i2cm_send(pn53x->transport_instance.i2cm, PN53X_I2C_ADDR, &request, size, true);
		if (result)
			return result;
		// check page 32 , get version request should be 00 FF 02 FE D4 02 2A
		bool acknack;
		result = pn53x_recv_ack(pn53x, &acknack);
		if (!acknack)
			return -1;

		memset (response,0,sizeof(*response));
		int retry_count = 0;
		while (!response->header.data_ready || result) {
			/// AH the problem my dear... size overflow right
			result =  bshal_i2cm_recv(pn53x->transport_instance.i2cm, PN53X_I2C_ADDR,response, sizeof(*response), false);
			retry_count++;
			if (retry_count >= 25)
				return -1;
		}
		if (result) return result;



		return 0;



		break;
	default:
		return -1;
	}
}

int pn53x_get_firmware_version(pn53x_t *pn53x, uint32_t *chip_id) {
	uint8_t request = 0x02;
	pn53x_i2c_spi_normal_frame_t response;
	int result = pn53x_send_frame (pn53x, &request, 1, &response);
	*chip_id = *(uint32_t*)(&response.frame.data[1]);
	return result;
}


int pn53x_find_card(pn53x_t *pn53x, picc_t *picc){
	uint8_t request [] = { 0x4A, 0x01, 0x00 };
	pn53x_i2c_spi_normal_frame_t response;
	picc->protocol = picc_protocol_iso14443a;
	int result = pn53x_send_frame (pn53x, request, sizeof(request), &response);
	if (result)
		return result;

	// I am getting an ACK from the PN532, but after that
	// I am not getting a data ready, so no data follows
	// I am expecting either card data, or no card found


	// response.frame.data[0] NbTg		Number of targets found, thus 0 = no card, 1 = card found
	// response.frame.data[1] Tg
	// response.frame.data[2] SNES_RES	I suppose this is ATQA as its 2 bytes
	// response.frame.data[3] SNES_RES  I suppose this is ATQA as its 2 bytes
	// response.frame.data[4] SEL_RES   I suppose this is SAK  as its 1 byte
	// response.frame.data[5] NFCIDLength	Length
	// response.frame.data[6+]  NFCID  = UUID
	// Followed by ATSLength, ATS


	if (response.frame.data[0] == 1) {
		// card found
		picc->atqa.as_uint8[0] = response.frame.data[2];
		picc->atqa.as_uint8[1] = response.frame.data[3];
		picc->sak.as_uint8 = response.frame.data[4];
		picc->size = response.frame.data[5];
		if (picc->size > 10) picc->size = 10; // Crop it if it is longer, should not occur
		memcpy(picc->uidByte, response.frame.data + 6, picc->size);
		return 0;
	} else {
		// no card found
		return -1; // TODO what to return?
	}

}


void PN53X_Init(pn53x_t *pn53x) {
}
