// Copyright (c) Thor Schueler. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _mcu_spi_magic_
#define _mcu_spi_magic_

#define write8(d) spi_write(d)
#define read8(dst) { dst=spi_read();}
#define setWriteDir() 
#define setReadDir()  

#define RD_ACTIVE   0
#define RD_IDLE     0
#define WR_ACTIVE   0
#define WR_IDLE     0

/** 
 * Pin usage as follow:
 *                   CS  DC/RS  RESET  SDI/MOSI  SCK   LED    VCC     GND    
 * ESP32             15   25     26       13     14    3.3V   3.3V    GND
 */

#define LED   -1            
#define RS    25       
#define RESET 26
#define CS    15
#define SID   13
#define SCK   14

#define WIDTH 320
#define HEIGHT 480

#define CD_COMMAND  (digitalWrite(RS,LOW))    
#define CD_DATA     (digitalWrite(RS,HIGH)) 
#define CS_ACTIVE   (digitalWrite(CS,LOW)) 
#define CS_IDLE     (digitalWrite(CS, HIGH)) 
#define MISO_STATE(x) { x = digitalRead(SID);}
#define MOSI_LOW    (digitalWrite(SID,LOW)) 
#define MOSI_HIGH   (digitalWrite(SID,HIGH)) 
#define CLK_LOW     (digitalWrite(SCK,LOW)) 
#define CLK_HIGH    (digitalWrite(SCK,HIGH)) 
#define WR_STROBE { }
#define RD_STROBE { }  

#define write16(d) write8(d>>8); write8(d)
#define read16(dst) { uint8_t hi; read8(hi); read8(dst); dst |= (hi << 8); }
#define writeCmd8(x) CD_COMMAND; write8(x)
#define writeData8(x)  CD_DATA; write8(x) 
#define writeCmd16(x)  CD_COMMAND; write16(x)
#define writeData16(x)  CD_DATA; write16(x)
#define writeData18(x)  CD_DATA; write8((x>>8)&0xF8); write8((x>>3)&0xFC); write8(x<<3)
#define writeCmdData8(a, d) CD_COMMAND; write8(a); CD_DATA; write8(d)
#define writeCmdData16(a, d)  CD_COMMAND; write8(a>>8); write8(a); CD_DATA; write8(d>>8); write8(d)

#endif // _mcu_spi_magic_
