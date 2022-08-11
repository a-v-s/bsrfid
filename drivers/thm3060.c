/*
 * thm3060.c
 *
 *  Created on: 11 aug. 2022
 *      Author: andre
 */

#include "thm3060.h"



int thm3060_recv(thm3060_t *thm3060, uint8_t reg, uint8_t *data, size_t amount) {
	int result = 0;
	reg &=~THM3060_DIR_WRITE;
	result = bshal_spim_transmit(thm3060->transport_instance.spim, &reg, 1, true);
	if (result) return result;
	return bshal_spim_receive(thm3060->transport_instance.spim, data, amount, false);
}

int thm3060_send(thm3060_t *thm3060, uint8_t reg, uint8_t *data, size_t amount) {
	int result = 0;
	reg |= THM3060_DIR_WRITE;
	result = bshal_spim_transmit(thm3060->transport_instance.spim, &reg, 1, true);
	if (result) return result;
	return bshal_spim_transmit(thm3060->transport_instance.spim, data, amount, false);
}

int thm3060_get_reg8(thm3060_t *thm3060, uint8_t reg, uint8_t *value) {
	return thm3060_recv(thm3060, reg , value, 1);
}

int thm3060_set_reg8(thm3060_t *thm3060, uint8_t reg, uint8_t value) {
	return thm3060_send(thm3060, reg , &value, 1);
}

int thm3060_or_reg8(thm3060_t *thm3060, uint8_t reg, uint8_t value) {
	uint8_t tmpval;
	int result;
	result = thm3060_get_reg8(thm3060, reg, &tmpval);
	if (result)
		return result;
	tmpval |= value;
	return thm3060_set_reg8(thm3060, reg, tmpval);
}
