/******************************************************************************\

File:         pn53x.h
Author:       André van Schoubroeck
License:      MIT

This implements the NXP PN53x family of RFID reader ICs

* NXP PN532
* NXP PN533
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

This file is a placeholder for planned support.

*******************************************************************************/

#include "pdc.h"
typedef bs_pdc_t pn53x_t;

int pn53x_get_firmware_version(pn53x_t *pn53x, uint32_t *chip_id) ;
