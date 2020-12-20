/******************************************************************************
 File:         mfrc52x_transport.c
 Author:       André van Schoubroeck
 License:      MIT

 This implements the transport protocols for MFRC52x family of RFID reader ICs.

 * SPI
 * I²C
 * UART

 The PN512 furthermore supports, but there won't be initially supported:

 * Parallel 8080 (Intel) style
 * Parallel 6800 (Motorola) style

 ********************************************************************************
 MIT License

 Copyright (c) 2020 André van Schoubroeck <andre@blaatschaap.be>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *******************************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "rc52x.h"

int mfrc522_recv(rc52x_t *rc52x, uint8_t reg, uint8_t *data, size_t amount) {
	uint8_t addr;
	int result = 0;
	//if (!mfrc_transmit) return -1;
	//if (!mfrc_receive) return -1;


	if (reg < 0x40) {
		switch (rc52x->transport) {
		case mfrc_transport_spi:
			if (!rc52x->transceive)
					return -1;
			addr = (reg << MFRC522_SPI_REG_SHIFT) | MFRC522_DIR_RECV;
			memset(data, addr, amount);
			break;
		case mfrc_transport_i2c:
			addr = reg;
			data[0] =addr;
			break;
		case mfrc_transport_uart:
			if (!rc52x->transceive)
								return -1;
			addr = reg | MFRC522_DIR_RECV;
			memset(data, addr, amount);
			break;
		default:
			return -1;
		}

	} else {
		// invalid address specified... but we're adding a feature here,
		// We'll read sequentially read the registers
		if (((reg & 0x3f) + amount) > 0x40)
			return -1;

		int tmpval;
		for (int i = 0; i < amount; i++) {
			switch (rc52x->transport) {
			case mfrc_transport_spi:
				tmpval = (((reg & 0x3f) + i) << MFRC522_SPI_REG_SHIFT)
						| MFRC522_DIR_RECV;
				if (i)
					data[i - 1] = tmpval;
				else
					addr = tmpval;
				break;
			case mfrc_transport_uart:
				tmpval = ((reg & 0x3f) + i) | MFRC522_DIR_RECV;
				if (i)
					data[i - 1] = tmpval;
				else
					addr = tmpval;
				break;
			}

		}
	}


		result = rc52x->transmit(&addr, 1, true);
		if (result)
			return result;
		if (rc52x->transport == mfrc_transport_i2c){
			return rc52x->receive(data, amount, false);
		} else {
			return rc52x->transceive(data, amount);
		}

}

int mfrc522_send(rc52x_t *rc52x, uint8_t reg, uint8_t *data, size_t amount) {
	int result = 0;
	uint8_t addr;
	switch (rc52x->transport) {
	case mfrc_transport_spi:
		addr = (reg << MFRC522_SPI_REG_SHIFT) | MFRC522_DIR_SEND;
		break;
	case mfrc_transport_i2c:
		addr = reg;
		break;
	case mfrc_transport_uart:
		addr = reg | MFRC522_DIR_SEND;
		break;
	default:
		return -1;
	}
	if (!rc52x->transmit)
		return -1;

	result = rc52x->transmit(&addr, 1, true);
	if (result)
		return result;
	result = rc52x->transmit(data, amount, false);
	return result;
}

int rc52x_card_transceive(rc52x_t *rc52x, uint8_t* sendbuff, size_t sendbytes, size_t sendbits, uint8_t *recvbuff, size_t recvbytes) {

}

int rc52x_get_reg8(rc52x_t *rc52x, uint8_t reg, uint8_t *value) {
	return mfrc522_recv(rc52x, reg, value, 1);
}

int rc52x_set_reg8(rc52x_t *rc52x, uint8_t reg, uint8_t value) {
	return mfrc522_send(rc52x, reg, &value, 1);
}

int rc52x_or_reg8(rc52x_t *rc52x, uint8_t reg, uint8_t value) {
	uint8_t tmpval;
	int result;
	result = rc52x_get_reg8(rc52x, reg, &tmpval);
	if (result)
		return result;
	tmpval |= value;
	return rc52x_set_reg8(rc52x, reg, tmpval);
}

int rc52x_and_reg8(rc52x_t *rc52x, uint8_t reg, uint8_t value) {
	uint8_t tmpval;
	int result;
	result = rc52x_get_reg8(rc52x, reg, &tmpval);
	if (result)
		return result;
	tmpval &= value;
	return rc52x_set_reg8(rc52x, reg, tmpval);
}

int rc52x_transport_init(rc52x_t *rc52x, mfrc_transport_t transport,
		mfrc_transport_transmit_f transmit, mfrc_transport_recveive_f receive,
		mfrc_transport_transceive_f transceive) {
	rc52x->transport = transport;
	rc52x->transmit = transmit;
	rc52x->receive = receive;
	rc52x->transceive = transceive;
	return 0;
}

