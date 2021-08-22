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
	if (!rc52x->transport_config)
		return -1;

	if (reg < 0x40) {
		switch (rc52x->transport) {
		case mfrc_transport_spi:
			addr = (reg << MFRC522_SPI_REG_SHIFT) | MFRC522_DIR_RECV;
			memset(data, addr, amount);
			break;
		case mfrc_transport_i2c:
			addr = reg;
			break;
		case mfrc_transport_uart:
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
	switch (rc52x->transport) {
	case mfrc_transport_spi:
		result = bshal_spim_transmit(rc52x->transport_config, &addr, 1, true);
		if (result)
			return result;
		return bshal_spim_transceive(rc52x->transport_config, data, amount);
		break;

	case mfrc_transport_i2c:
		//!! TODO: I²C ADDRESS
		result = bshal_i2cm_recv_reg(rc52x->transport_config, 0x28, reg, data, amount);
		return result;
		break;
	default:
		return -1;
	}
}

int mfrc522_send(rc52x_t *rc52x, uint8_t reg, uint8_t *data, size_t amount) {
	int result = 0;
	uint8_t addr;
	if (!rc52x->transport_config)
		return -1;

	switch (rc52x->transport) {
	case mfrc_transport_spi:
		addr = (reg << MFRC522_SPI_REG_SHIFT) | MFRC522_DIR_SEND;
		result = bshal_spim_transmit(rc52x->transport_config, &addr, 1, true);
		if (result)
			return result;
		result = bshal_spim_transmit(rc52x->transport_config, data, amount,
				false);
		return result;
		break;
	case mfrc_transport_i2c:
		//!! TODO: I²C ADDRESS
		result = bshal_i2cm_send_reg(rc52x->transport_config, 0x28, reg, data, amount);
		return result;
		break;
	case mfrc_transport_uart:
		addr = reg | MFRC522_DIR_SEND;
		break;
	default:
		return -1;
	}

	return -1;
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

