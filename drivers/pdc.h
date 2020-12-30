/*
 * pdc.h
 *
 *  Created on: 29 dec. 2020
 *      Author: andre
 */

#ifndef BSRFID_DRIVERS_PDC_H_
#define BSRFID_DRIVERS_PDC_H_

typedef int(*delay_ms_f)(int ms);

typedef enum {
	STATUS_OK				= 0,	// Success
	STATUS_ERROR			= -1,	// Error in communication
	STATUS_HARD_ERROR		= -2,	// Hardware error (eg. I2C NACK)
	STATUS_COLLISION		= -3,	// Collission detected
	STATUS_TIMEOUT			= -4,	// Timeout in communication.
	STATUS_NO_ROOM			= -5,	// A buffer is not big enough.
	STATUS_INTERNAL_ERROR	= -6,	// Internal error in the code. Should not happen ;-)
	STATUS_INVALID			= -7,	// Invalid argument.
	STATUS_CRC_WRONG		= -8,	// The CRC_A does not match
	STATUS_MIFARE_NACK		= -9,		// A MIFARE PICC responded with NAK.
} rc52x_result_t;


typedef enum {
    mfrc_transport_undefined,
    mfrc_transport_spi,
    mfrc_transport_i2c,
    mfrc_transport_uart,
    mfrc_transport_8080,
    mfrc_transport_6800,
} mfrc_transport_t;



typedef int (*TransceiveData_f)(void *pdc, uint8_t *sendData, size_t sendLen,
		uint8_t *backData, size_t *backLen, uint8_t *validBits,
		uint8_t rxAlign, uint8_t *collisionPos, bool sendCRC, bool recvCRC);

typedef int (*SetBitFraming_f)(void *pdc, int rxAlign, int txLastBits);

typedef struct {
	mfrc_transport_t transport;
	void* transport_config;
	delay_ms_f delay_ms;
	TransceiveData_f TransceiveData;
	//SetBitFraming_f SetBitFraming;
} bs_pdc_t;




#endif /* BSRFID_DRIVERS_PDC_H_ */
