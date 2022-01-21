/******************************************************************************
 File:         mfrc66x_transport.c
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

#include "rc66x.h"

int rc66x_recv(rc66x_t *rc66x, uint8_t reg, uint8_t *data, size_t amount) {
	uint8_t addr;
	int result = 0;
	if (!rc66x->transport_instance.raw)
		return -1;


		switch (rc66x->transport_type) {
		case bshal_transport_spi:
		case bshal_transport_uart:
			addr = (reg << RC66X_SPI_REG_SHIFT) | RC66X_DIR_RECV;
			memset(data, addr, amount);
			break;
		case bshal_transport_i2c:
			addr = reg;
			break;
		default:
			return -1;
		}


	switch (rc66x->transport_type) {
	case bshal_transport_spi:
		result = bshal_spim_transmit(rc66x->transport_instance.spim, &addr, 1, true);
		if (result)
			return result;
		return bshal_spim_transceive(rc66x->transport_instance.spim, data, amount, false);
		break;
	default:
		return -1;
	}
}

int rc66x_send(rc66x_t *rc66x, uint8_t reg, uint8_t *data, size_t amount) {
	int result = 0;
	uint8_t addr;
	if (!rc66x->transport_instance.raw)
		return -1;

	switch (rc66x->transport_type) {
	case bshal_transport_spi:
		addr = (reg << RC66X_SPI_REG_SHIFT) | RC66X_DIR_SEND;
		result = bshal_spim_transmit(rc66x->transport_instance.spim, &addr, 1, true);
		if (result)
			return result;
		result = bshal_spim_transmit(rc66x->transport_instance.spim, data, amount,
				false);
		return result;
		break;
	case bshal_transport_i2c:
		addr = reg;
		break;
	case bshal_transport_uart:
		addr = reg | RC66X_DIR_SEND;
		break;
	default:
		return -1;
	}

	return -1;
}

int rc66x_get_reg8(rc66x_t *rc66x, uint8_t reg, uint8_t *value) {
	return rc66x_recv(rc66x, reg, value, 1);
}

int rc66x_set_reg8(rc66x_t *rc66x, uint8_t reg, uint8_t value) {
	return rc66x_send(rc66x, reg, &value, 1);
}

int rc66x_or_reg8(rc66x_t *rc66x, uint8_t reg, uint8_t value) {
	uint8_t tmpval;
	int result;
	result = rc66x_get_reg8(rc66x, reg, &tmpval);
	if (result)
		return result;
	tmpval |= value;
	return rc66x_set_reg8(rc66x, reg, tmpval);
}

int rc66x_and_reg8(rc66x_t *rc66x, uint8_t reg, uint8_t value) {
	uint8_t tmpval;
	int result;
	result = rc66x_get_reg8(rc66x, reg, &tmpval);
	if (result)
		return result;
	tmpval &= value;
	return rc66x_set_reg8(rc66x, reg, tmpval);
}

