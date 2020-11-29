/******************************************************************************\

File:         st25r39.h
Author:       André van Schoubroeck
License:      MIT

This implements the STM ST25R95 family of RFID reader ICs

* STM CR95HF
* STM ST95HF
* STM ST25R95
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


https://community.st.com/s/question/0D53W00000Bsjx4SAB/differences-between-st95hf-and-st25r95

Differences:
+----------+-----+------+----------------+
| Device   | SPI | Uart | Card Emulation |
+----------+-----+------+----------------+ 
| CR95HF   | Yes | Yes  | No             | 
| ST95HF   | Yes | No   | Yes            | 
| ST25R95  | Yes | No   | Yes            | 
+----------+-----+------+----------------+

*******************************************************************************/
