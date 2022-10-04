/*
 * rc52x_ref.h
 *
 *  Created on: 9 sep. 2022
 *      Author: andre
 */

#ifndef BSRFID_DRIVERS_RC52X_REF_H_
#define BSRFID_DRIVERS_RC52X_REF_H_

#include <stdint.h>

extern const uint8_t rc52x_ref_mfrc522_V0[];
extern const uint8_t rc52x_ref_mfrc522_V1[];
extern const uint8_t rc52x_ref_mfrc522_V2[];
extern const uint8_t rc52x_ref_mfrc523_V1[];
extern const uint8_t rc52x_ref_mfrc523_V2[];
extern const uint8_t rc52x_ref_pn512_v1[];
extern const uint8_t rc52x_ref_pn512_v2[];
extern const uint8_t rc52x_ref_fm17522[];

typedef enum {
	rc52x_ref_status_success,
	rc52x_ref_status_fail,
	rc52x_ref_status_unknown,
	rc52x_ref_status_error,
} rc52x_ref_status_t;

#endif /* BSRFID_DRIVERS_RC52X_REF_H_ */
