/******************************************************************************
 File:         pn5180.c
 Author:       André van Schoubroeck
 License:      MIT

 This implements the PN5180 family of RFID reader ICs

 * PN5180
 * Other compatibles
 ********************************************************************************
 MIT License

 Copyright (c) 2020-2022 André van Schoubroeck <andre@blaatschaap.be>

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
 ********************************************************************************


 Placeholder for planned future implementation.

 *******************************************************************************/

#include "pn5180.h"

int pn5180_get_reg32(pn5180_t *pn5180, uint8_t reg, uint32_t *value) {
	pn5180_request_t req = { 0 };
	req.command = PN5180_CMD_READ_REGISTER;
	req.reg.address = reg;
	int result = bshal_spim_transmit(pn5180->transport_instance.spim, &req, 2,
			false);
	if (result)
		return result;
	bshal_delay_ms(1);
	result = bshal_spim_receive(pn5180->transport_instance.spim, value,
			sizeof(uint32_t), false);
	return result;
}

int pn5180_set_reg32(pn5180_t *pn5180, uint8_t reg, uint32_t value) {
	pn5180_request_t req = { 0 };
	req.command = PN5180_CMD_WRITE_REGISTER;
	req.reg.address = reg;
	req.reg.value = value;
	int result = bshal_spim_transmit(pn5180->transport_instance.spim, &req, 6,
			false);
	return result;
}

int pn5180_or_reg32(pn5180_t *pn5180, uint8_t reg, uint32_t value) {
	pn5180_request_t req = { 0 };
	req.command = PN5180_CMD_WRITE_REGISTER_OR_MASK;
	req.reg.address = reg;
	req.reg.value = value;
	int result = bshal_spim_transmit(pn5180->transport_instance.spim, &req, 6,
			false);
	return result;
}
int pn5180_and_reg32(pn5180_t *pn5180, uint8_t reg, uint32_t value) {
	pn5180_request_t req = { 0 };
	req.command = PN5180_CMD_WRITE_REGISTER_AND_MASK;
	req.reg.address = reg;
	req.reg.value = value;
	int result = bshal_spim_transmit(pn5180->transport_instance.spim, &req, 6,
			false);
	return result;

}

int pn5180_test(pn5180_t *pdc) {
	pn5180_eeprom_info eeprom = { 0 };
	pn5180_request_t req = { 0 };
	req.command = PN5180_CMD_READ_EEPROM;
	req.read_eeprom.start_address = 0;
	req.read_eeprom.number_of_bytes = sizeof(pn5180_eeprom_info);

	// While the datasheet says I should keep the chip select low during a transaction
	// I actually must toggle it between read and write to get a result.
	// Otherwise it results all 0xFF
	//int result = bshal_spim_transmit(pdc->transport_instance.spim, &req, 3, true);
	int result = bshal_spim_transmit(pdc->transport_instance.spim, &req, 3,
			false);
	bshal_delay_ms(1);
	if (result)
		return result;
	result = bshal_spim_receive(pdc->transport_instance.spim, &eeprom,
			sizeof(pn5180_eeprom_info), false);
	bshal_delay_ms(1);


	return result;

}

void PN5180_Init(pn5180_t *pn5180) {
	pn5180_request_t req = { 0 };
	int result;
	//  AN12650 - "Using the PN5180 without library"1
	//	1: sendSPI(0x11, 0x00, 0x80);
	//  1: Loads the ISO 14443 - 106 protocol into the RF registers
	req.command = PN5180_CMD_LOAD_RF_CONFIG;
	req.raw[0] = 0x00;
	req.raw[1] = 0x80;
	result = bshal_spim_transmit(pn5180->transport_instance.spim, &req, 3,
			false);
	if (result)
		return result;
	bshal_delay_ms(1);
	pn5180_test(pn5180);


	pn5180_or_reg32(pn5180, 0x04, 0x01);
	bshal_delay_ms(1);


	//	2: sendSPI(0x16, 0x00);
	//  2: Switches the RF field ON.
	//  So after turning the RF field on, it stops responding
	//  Could we have a brownout problem?
	req.command = PN5180_CMD_RF_ON;
	req.raw[0] = 0x00;
	result = bshal_spim_transmit(pn5180->transport_instance.spim, &req, 2,
			false);
	if (result)
		return result;
	bshal_delay_ms(10);
	pn5180_test(pn5180);






	//	3: sendSPI(0x02, 0x19, 0xFE, 0xFF, 0xFF, 0xFF);
	//  3: Switches the CRC extension off in Tx direction
	// Note assuming little endian host
	// TODO: register defines
	pn5180_and_reg32(pn5180, 0x19, 0xFFFFFFFE);
	bshal_delay_ms(1);
	pn5180_test(pn5180);

	//	4: sendSPI(0x02, 0x12, 0xFE, 0xFF, 0xFF, 0xFF);
	//  4: Switches the CRC extension off in Rx direction
	// Note assuming little endian host
	// TODO: register defines
	pn5180_and_reg32(pn5180, 0x12, 0xFFFFFFFE);
	bshal_delay_ms(1);
	pn5180_test(pn5180);



	uint32_t irq_status = 0;
	result = pn5180_get_reg32(pn5180, 0x02, &irq_status);



	//	5: sendSPI(0x00, 0x03, 0xFF, 0xFF, 0x0F, 0x00);
	//  5: Clears the interrupt register IRQ_STATUS
	// Note assuming little endian host
	// TODO: register defines
	pn5180_set_reg32(pn5180, 0x03, 0x000fffff);
	bshal_delay_ms(1);
	pn5180_test(pn5180);

	//	6: sendSPI(0x02, 0x00, 0xF8, 0xFF, 0xFF, 0xFF);
	//  6: Sets the PN5180 into IDLE state
	// Note assuming little endian host
	// TODO: register defines
	pn5180_and_reg32(pn5180, 0x00, 0xfffffff8);
	bshal_delay_ms(1);
	pn5180_test(pn5180);

	//	7: sendSPI(0x01, 0x00, 0x03, 0x00, 0x00, 0x00);
	//  7: Activates TRANSCEIVE routine
	// Note assuming little endian host
	// TODO: register defines
	pn5180_or_reg32(pn5180, 0x00, 0x00000003);
	bshal_delay_ms(1);
	pn5180_test(pn5180);


	//	8: sendSPI(0x09, 0x07, 0x26);
	//  8: Sends REQA command
	req.command = PN5180_CMD_SEND_DATA;
	req.raw[0] = 0x07;
	req.raw[1] = 0x26;
	result = bshal_spim_transmit(pn5180->transport_instance.spim, &req, 3,
			false);
	if (result)
		return result;
	bshal_delay_ms(1);
	pn5180_test(pn5180);

	//	9: waitForCardResponse();
	//  9: Waits until a Card has responded via checking the IRQ_STATUS register
	irq_status = 0;
	while (!irq_status) {
		result = pn5180_get_reg32(pn5180, 0x02, &irq_status);
		if (result)
			return result;
		bshal_delay_ms(1);
		pn5180_test(pn5180);
	}

	return 0;

	//	10: sendSPI(0x0A, 0x00);
	//  10: Reads the reception buffer. (ATQA)


	//	11: sendSPI(0x17, 0x00);
	//  11: Switches the RF field OFF.
}
