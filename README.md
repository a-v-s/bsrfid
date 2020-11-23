libbsrfid
=========

Yet another RFID reader library for microcontrollers written in C.


Plans
=====

* The MFRC522 family
* The PN532 family
* When done, others might be added

Why? 
===

The RC522 and PN532 are popular RFID chips sold by many Chinese sellers on 
eBay and AliExpress. They are popular amongst hobbiest, and therefore there 
are plenty of implementations for them out there. So, why writing another? 

Most of the MFRC522 boards sold at eBay and AliExpress are hard-wired for SPI,
therefore, most of the libraries out there only support SPI as a transport. I
intend to also provide an implementation for I²C and UART.

Furthermore, there are some chips related to the MFRC522 and PN512. These chips
use the same protocol, but provide additional features. I might take a look at
those, provided I get the hardware.

Next, there is the PN532. The boards sold on eBay and AliExpress provide 
dip switches to select the protocol. The main intend here is to provide an API 
to support both chips, and extend it to other chips, 
such as THM3060 and CLRC663.
