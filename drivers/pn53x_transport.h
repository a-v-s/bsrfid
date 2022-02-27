/*
 * pn53x_transport.h
 *
 *  Created on: 27 feb. 2022
 *      Author: andre
 */

#ifndef BSRFID_DRIVERS_PN53X_TRANSPORT_H_
#define BSRFID_DRIVERS_PN53X_TRANSPORT_H_

#define PN53X_I2C_ADDR	(0x24)

#define PN53X_START_CODE 		0xFF00 // NB LITTLE ENDIAN
#define PN53X_DIR_TO_PN53X 		0xD4
#define PN53X_DIR_FROM_PN53X 	0xD5

#define PN53X_ACK				0xFF00 // NB LITTLE ENDIAN
#define PN53X_NACK				0x00FF // NB LITTLE ENDIAN

#pragma pack(push,1)
typedef struct {
	uint16_t start_code; // 0xff00 le
	uint8_t len;
	uint8_t lcs;
	uint8_t tfi; // direction 0xD4 host to pn, 0xd5 pn to host
	uint8_t data[64]; // data + checksum // Please note the sizes here,
} pn53x_normal_frame_t;

typedef struct {
	uint16_t start_code;
	uint16_t ack_or_nack;
} pn53x_ack_nack_frame_t;

typedef struct {
	uint8_t data_ready;
	uint8_t preamble;
} pn53x_i2c_spi_response_header_t;

typedef struct {
	pn53x_i2c_spi_response_header_t header;
	pn53x_ack_nack_frame_t frame;
} pn53x_i2c_spi_ack_nack_frame_t;


typedef struct {
	pn53x_i2c_spi_response_header_t header;
	pn53x_normal_frame_t frame;
} pn53x_i2c_spi_normal_frame_t;



#pragma pack(pop)
#endif /* BSRFID_DRIVERS_PN53X_TRANSPORT_H_ */
