/******************************************************************************\

File:         thm3060.h
Author:       André van Schoubroeck
License:      MIT

This implements the RC522 family of RFID reader ICs

* THM3060
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




*******************************************************************************/

#include "pdc.h"
typedef bs_pdc_t thm3060_t;

#define THM3060_DIR_READ	(0x00)
#define THM3060_DIR_WRITE	(0x80)

#define THM3060_REG_DATA	(0x00)
#define THM3060_REG_PSEL	(0x01)
#define THM3060_REG_FCONB	(0x02)
#define THM3060_REG_EGT		(0x03)
#define THM3060_REG_CRCSEL	(0x04)
#define THM3060_REG_RSTAT	(0x05)
#define THM3060_REG_SCON	(0x06)
#define THM3060_REG_INTCON	(0x07)
#define THM3060_REG_RSCH	(0x08)
#define THM3060_REG_RSCL	(0x09)
#define THM3060_REG_CRCH	(0x0A)
#define THM3060_REG_CRCL	(0x0B)
#define THM3060_REG_TMRH	(0x0C)
#define THM3060_REG_TMRL	(0x0D)
#define THM3060_REG_BPOS	(0x0E)
#define THM3060_REG_SMOD	(0x10)
#define THM3060_REG_PWTH	(0x11)

