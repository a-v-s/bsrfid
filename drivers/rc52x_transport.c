/******************************************************************************\

File:         mfrc52x_transport.c
Author:       André van Schoubroeck
License:      MIT

This implements the transport protocols for MFRC52x family of RFID reader ICs.

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

#include "mfrc52x_transport.h"

static mfrc_transport_t m_transport = mfrc_transport_undefined;
static mfrc_transport_send_f mfrc_send = NULL;
static mfrc_transport_recv_f mfrc_recv = NULL;



int mfrc522_recv(uint8_t reg, uint8_t *data, size_t amount){
    uint8_t addr;
    int result = 0;
    if (!mfrc_transport_send) return -1;
    if (!mfrc_transport_recv) return -1;

    if (reg < 0x40) {
        switch(m_transport) {
            case mfrc_transport_spi:
                addr = (reg << MFRC522_SPI_REG_SHIFT) | MFRC522_DIR_RECV;
                memset(data, addr, amount);
                break;
            case mfrc_transport_i2c:
                addr = reg;
                break;
            case mfrc_transport_uart:
                addr = reg | MFRC522_DIR_RECV;
                memset(data, addr, amount);
                break;
            default:
                return -1;
            }
        }
    } else {
        // invalid address specified... but we're adding a feature here, 
        // We'll read sequentially read the registers 
        if (((reg&0x3f) + amount) > 0x40 ) return -1; 

        for (int i = 0; i < amount; i++) {
            switch(m_transport) {
                case mfrc_transport_spi:
                    addr = (((reg&0x3f)+i) << MFRC522_SPI_REG_SHIFT) | MFRC522_DIR_RECV;
                    break;
                case mfrc_transport_uart:
                    addr = ((reg&0x3f)+i) | MFRC522_DIR_RECV;
                    break;
            }
            data[i] = addr;
        }
    }

    

    // Now let's have a trasceive function

    


}


int mfrc522_send(uint8_t reg, uint8_t *data, size_t amount){
    uint8_t addr;
    switch(m_transport) {
        case mfrc_transport_spi:
            addr = (reg << MFRC522_SPI_REG_SHIFT) | MFRC522_DIR_SEND;
            break;
        case mfrc_transport_i2c:
            addr = reg;
            break;
        case mfrc_transport_uart:
            addr = reg | MFRC522_DIR_SEND;
            break;
        default:
            return -1;
    }
    if (!mfrc_transport_send) return -1;
    if (!mfrc_transport_recv) return -1;
    result = mfrc_transport_send(&addr, 1, true);
    if (result) return result;
    result = mfrc_transport_send(data, amount, false);
    if (result) return result;
}
