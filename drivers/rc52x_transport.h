/******************************************************************************\

File:         mfrc52x_transport.h
Author:       André van Schoubroeck
License:      MIT

This implements the transport protocols for RC522 family of RFID reader ICs.

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


#define MFRC522_DIR_RECV        (0x80)
#define MFRC522_DIR_SEND        (0x00)
#define MFRC522_SPI_REG_SHIFT   (1) 

typedef void(mfrc_transport_send_f*)(uint8_t *data, size_t amount, bool nostop);
typedef void(mfrc_transport_recv_f*)(uint8_t *data, size_t amount, bool nostop);

typedef enum {
    mfrc_transport_undefined,
    mfrc_transport_spi,
    mfrc_transport_i2c,
    mfrc_transport_uart,
    mfrc_transport_8080,
    mfrc_transport_6800,
} mfrc_transport_t;


int mfrc522_recv(uint8_t reg, uint8_t *data, size_t amount);
int mfrc522_send(uint8_t reg, uint8_t *data, size_t amount);



