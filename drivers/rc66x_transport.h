/*
 * rc66x_transport.h
 *
 *  Created on: 30 dec. 2022
 *      Author: andre
 */

#ifndef ESP32_ESP_IDF_COMPONENTS_UCDEV_BSRFID_DRIVERS_RC66X_TRANSPORT_H_
#define ESP32_ESP_IDF_COMPONENTS_UCDEV_BSRFID_DRIVERS_RC66X_TRANSPORT_H_

int rc66x_recv(rc66x_t *rc66x, uint8_t reg, uint8_t *data, size_t amount);
int rc66x_send(rc66x_t *rc66x, uint8_t reg, uint8_t *data, size_t amount);
int rc66x_get_reg8(rc66x_t *rc66x, uint8_t reg, uint8_t *value);
int rc66x_set_reg8(rc66x_t *rc66x, uint8_t reg, uint8_t value);
int rc66x_or_reg8(rc66x_t *rc66x, uint8_t reg, uint8_t value);
int rc66x_and_reg8(rc66x_t *rc66x, uint8_t reg, uint8_t value);

#endif /* ESP32_ESP_IDF_COMPONENTS_UCDEV_BSRFID_DRIVERS_RC66X_TRANSPORT_H_ */
