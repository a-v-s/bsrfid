/*
 * ntag.c
 *
 *  Created on: 22 dec. 2020
 *      Author: andre
 */





/**
 * Authenticate with a NTAG216.
 *
 * Only for NTAG216. First implemented by Gargantuanman.
 *
 * @param[in]   passuint16_t   passuint16_t.
 * @param[in]   pACK       result success???.
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
rc52x_result_t PCD_NTAG216_AUTH(rc52x_t *rc52x, uint8_t *passuint16_t,
		uint8_t pACK[]) //Authenticate with 32bit passuint16_t
{
	// TODO: Fix cmdBuffer length and rxlength. They really should match.
	//       (Better still, rxlength should not even be necessary.)

	rc52x_result_t result;
	uint8_t cmdBuffer[18]; // We need room for 16 uint8_ts data and 2 uint8_ts CRC_A.

	cmdBuffer[0] = 0x1B; //Comando de autentificacion

	for (uint8_t i = 0; i < 4; i++)
		cmdBuffer[i + 1] = passuint16_t[i];

	result = PCD_CalculateCRC(rc52x, cmdBuffer, 5, &cmdBuffer[5]);

	if (result != STATUS_OK) {
		return result;
	}

	// Transceive the data, store the reply in cmdBuffer[]
	uint8_t waitIRq = 0x30;	// RxIRq and IdleIRq
//	uint8_t cmdBufferSize	= sizeof(cmdBuffer);
	uint8_t validBits = 0;
	uint8_t rxlength = 5;

	result = PCD_CommunicateWithPICC(rc52x, RC52X_CMD_Transceive, waitIRq,
			cmdBuffer, 7, cmdBuffer, &rxlength, &validBits, 0, false);

	pACK[0] = cmdBuffer[0];
	pACK[1] = cmdBuffer[1];

	if (result != STATUS_OK) {
		return result;
	}

	return STATUS_OK;
} // End PCD_NTAG216_AUTH()
