/******************************************************************************\

File:         rc52x.h
Author:       André van Schoubroeck
License:      MIT

This implements the RC522 family of RFID reader ICs

* NXP MFRC522         (ISO/IEC14443A)
* NXP MFRC523         (ISO/IEC14443A,ISO/IEC14443B)
* NXP PN512           (ISO/IEC14443A,ISO/IEC14443B, FeliCa)
* FUDAN FM17522E      (ISO/IEC14443A)
* Other compatibles 
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
********************************************************************************


MFRC522 only supports ISO/IEC14443B
MFRC523 also supports ISO/IEC14443B
  PN512 also supports FeliCa. (Are they ISO/IEC18092 ?)

As the MFRC522 is the commonly available hardware on eBay and AliExpress, that
is the hardware I have available. Support for the features of the other chips
will be limited due missing hardware. 

*******************************************************************************/

#ifndef _MFCR522_H_
#define _MFCR522_H_



#include "rc52x_transport.h"

typedef struct {
	mfrc_transport_t transport;
	mfrc_transport_transmit_f transmit;
	mfrc_transport_recveive_f receive;
	mfrc_transport_transceive_f transceive;
} rc52x_t;








//------------------------------------------------------------------------------
// Regisers
// -----------------------------------------------------------------------------
// These are the registers for all of RC522, RC523 and PN512. Some registers
// may be reserved on some of these chips.
// -----------------------------------------------------------------------------

#define MFRC_REG_CommandReg         (0x01)
#define MFRC_REG_ComlEnReg         	(0x02)
#define MFRC_REG_DivlEnReg          (0x03)
#define MFRC_REG_ComIrqReg          (0x04)
#define MFRC_REG_DivIrqReg          (0x05)
#define MFRC_REG_ErrorReg           (0x06)
#define MFRC_REG_Status1Reg         (0x07)
#define MFRC_REG_Status2Reg         (0x08)
#define MFRC_REG_FIFODataReg        (0x09)
#define MFRC_REG_FIFOLevelReg       (0x0A)
#define MFRC_REG_WaterLevelReg      (0x0B)
#define MFRC_REG_ControlReg         (0x0C)
#define MFRC_REG_BitFramingReg      (0x0D)
#define MFRC_REG_CollReg            (0x0E)

#define MFRC_REG_ModeReg            (0x11)
#define MFRC_REG_TxModeReg          (0x12)
#define MFRC_REG_RxModeReg          (0x13)
#define MFRC_REG_TxControlReg       (0x14)
#define MFRC_REG_TxASKReg           (0x15)
#define MFRC_REG_TxSelReg           (0x16)
#define MFRC_REG_RxSelReg           (0x17)
#define MFRC_REG_RxThresholdReg     (0x18)
#define MFRC_REG_DemodReg           (0x19)
#define MFRC_REG_FelNFC1Reg         (0x1A)
#define MFRC_REG_FelNFC2Reg         (0x1B)
#define MFRC_REG_MfTxReg            (0x1C)
#define MFRC_REG_MfRxReg            (0x1D)
#define MFRC_REG_TypeBReg           (0x1E)
#define MFRC_REG_SerialSpeedReg     (0x1F)

#define MFRC_REG_CRCResultReg_Hi    (0x21)
#define MFRC_REG_CRCResultReg_Lo    (0x22)
#define MFRC_REG_GsNOffReg          (0x23)
#define MFRC_REG_ModWidthReg        (0x24)
#define MFRC_REG_TxBitPhaseReg      (0x25)
#define MFRC_REG_RFCfgReg           (0x26)
#define MFRC_REG_GsNOnReg           (0x27)
#define MFRC_REG_CWGsPReg           (0x28)
#define MFRC_REG_ModGsPReg          (0x29)
#define MFRC_REG_TModeReg           (0x2A)
#define MFRC_REG_TPrescalerReg      (0x2B)
#define MFRC_REG_TReloadReg_Hi      (0x2C)
#define MFRC_REG_TReloadReg_Lo      (0x2D)
#define MFRC_REG_TCounterVal_Hi     (0x2E)
#define MFRC_REG_TCounterVal_Lo     (0x2F)

#define MFRC_REG_TestSel1Reg        (0x31)
#define MFRC_REG_TestSel2Reg        (0x32)
#define MFRC_REG_TestPinEnReg       (0x33)
#define MFRC_REG_TestPinValueReg    (0x34)
#define MFRC_REG_TestBusReg         (0x35)
#define MFRC_REG_AutoTestReg        (0x36)
#define MFRC_REG_VersionReg         (0x37)
#define MFRC_REG_AnalogTestReg      (0x38)
#define MFRC_REG_TestDAC1Reg        (0x39)
#define MFRC_REG_TestDAC2Reg        (0x3A)
#define MFRC_REG_TestADCReg         (0x3B)


//------------------------------------------------------------------------------
// Register Helper
// -----------------------------------------------------------------------------
// Define sequential mode and 16 bit registers. (NB Big Endian) 
// -----------------------------------------------------------------------------

#define MFRC_REG_SequentialAddr     (0x40)
#define MFRC_REG_CRCResultReg       (MFRC_SequentialAddr + MFRC_CRCResultReg_Hi)
#define MFRC_REG_TCounterVal        (MFRC_SequentialAddr + MFRC_TCounterVal_Hi)
#define MFRC_REG_TReloadReg			(MFRC_SequentialAddr + MFRC_REG_TReloadReg_Hi)


//------------------------------------------------------------------------------
// Commands
// -----------------------------------------------------------------------------
// These are the commands for all of RC522, RC523 and PN512. Some commands
// may be reserved on some of these chips.
// -----------------------------------------------------------------------------

#define MFRC_Idle               (0b0000)
#define MFRC_Configure          (0b0001)
#define MFRC_GenerateRandomID   (0b0010)
#define MFRC_CalcCRC            (0b0011)
#define MFRC_Transmit           (0b0100)
#define MFRC_NoCmdChange        (0b0111)    
#define MFRC_Receive            (0b1000)
#define MFRC_Transceive         (0b1100)
#define MFRC_AutoColl           (0b1101)
#define MFRC_MFAuthent          (0b1110)
#define MFRC_SoftReset          (0b1111)


#endif // _MFCR522_H_


