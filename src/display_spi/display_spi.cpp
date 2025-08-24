
// Copyright (c) Thor Schueler. All rights reserved.
// SPDX-License-Identifier: MIT
// IMPORTANT: LIBRARY MUST BE SPECIFICALLY CONFIGURED FOR EITHER TFT SHIELD
// OR BREAKOUT BOARD USAGE.

#include <SPI.h>
#include "../logging/SerialLogger.h"
#include "display_spi.h"
#include "lcd_spi_registers.h"
#include "mcu_spi_magic.h"

#define TFTLCD_DELAY16  0xFFFF
#define TFTLCD_DELAY8   0x7F
#define MAX_REG_NUM     24
#define swap(a, b) { int16_t t = a; a = b; b = t; }


static const uint8_t display_buffer[TFT_WIDTH * TFT_HEIGHT * 2] PROGMEM = {0};
static const uint8_t PROGMEM initcmd[] = {
	0xEF, 3, 0x03, 0x80, 0x02,
	0xCF, 3, 0x00, 0xC1, 0x30,
	0xED, 4, 0x64, 0x03, 0x12, 0x81,
	0xE8, 3, 0x85, 0x00, 0x78,
	0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
	0xF7, 1, 0x20,
	0xEA, 2, 0x00, 0x00,
	ILI9341_PWCTR1  , 1, 0x23,             // Power control VRH[5:0]
	ILI9341_PWCTR2  , 1, 0x10,             // Power control SAP[2:0];BT[3:0]
	ILI9341_VMCTR1  , 2, 0x3e, 0x28,       // VCM control
	ILI9341_VMCTR2  , 1, 0x86,             // VCM control2
	ILI9341_MADCTL  , 1, 0x48,             // Memory Access Control
	ILI9341_VSCRSADD, 1, 0x00,             // Vertical scroll zero
	ILI9341_PIXFMT  , 1, 0x55,
	ILI9341_FRMCTR1 , 2, 0x00, 0x18,
	ILI9341_DFUNCTR , 3, 0x08, 0x82, 0x27, // Display Function Control
	0xF2, 1, 0x00,                         // 3Gamma Function Disable
	ILI9341_GAMMASET , 1, 0x01,             // Gamma curve selected
	ILI9341_GMCTRP1 , 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, // Set Gamma
		0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
	ILI9341_GMCTRN1 , 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, // Set Gamma
		0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
	ILI9341_SLPOUT  , 0x80,                // Exit Sleep
	ILI9341_DISPON  , 0x80,                // Display on
	0x00                                   // End of list
};


/**
 * @brief Generates a new instance of the DISPLAY_SPI class. 
 * @details initializes the SPI and LCD pins including CS, RS, RESET 
 */
DISPLAY_SPI::DISPLAY_SPI() {}

#pragma region public methods
/**
 * @brief Pass 8-bit (each) R,G,B, get back 16-bit packed color
 * @param r - Red color value
 * @param g - Green color value
 * @param b - Blue color value
 * @returns 16-bit packed color value
 */
uint16_t DISPLAY_SPI::RGB_to_565(uint8_t r, uint8_t g, uint8_t b)
{
	return ((r& 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}

/**
 * @brief draw backgound image on the display
 * @param image - array to image containing 565 color values per pixel
 * @param size - the number of elements in the image (should be wxhx3)
 */
void DISPLAY_SPI::draw_background(const unsigned char* image, size_t size)
{
  SPI_BEGIN_TRANSACTION();
	CS_ACTIVE;
	set_addr_window(0, 0, width, height);
	writeCommand(ILI9341_MEMORYWRITE);
	CD_DATA;
	spi->transferBytes(image, nullptr, size);
	CS_IDLE;
  SPI_END_TRANSACTION();
}

/**
 * @brief draw image on the display
 * @param image - array to image containing 565 color values per pixel
 * @param size - the number of elements in the image (should be wxhx3)
 * @param x - starting x coordinate
 * @param y - starting y coordinate
 * @param w - image width
 * @param h - image height
 */
void DISPLAY_SPI::draw_image(const unsigned char* image, size_t size, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  SPI_BEGIN_TRANSACTION();
	CS_ACTIVE;
	set_addr_window(x, y, w, h);
	writeCommand(ILI9341_MEMORYWRITE);
	CD_DATA;
	spi->transferBytes(image, nullptr, size);
	CS_IDLE;
  SPI_END_TRANSACTION();
}

/**
 * @brief Draws a pixel of a certain color at a certain location
 * @param x - x coordinate of the pixel
 * @param y - y coordinate of the pixel
 * @param color - the color of the pixel to set
 */
void DISPLAY_SPI::draw_pixel(int16_t x, int16_t y, uint16_t color)
{
  // Clip first...
  if ((x >= 0) && (x < width) && (y >= 0) && (y < height)) {
    // THEN set up transaction (if needed) and draw...
    START_WRITE();
    set_addr_window(x, y, 1, 1);
    SPI_WRITE16(color);
    END_WRITE();
  }
}

/**
 * @brief Draws a horizontal line on the screen
 * @param x - x coordinate of the start
 * @param y - y coordinate of the start
 * @param w - the width of the line
 * @param color - the color to set
 */
void DISPLAY_SPI::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) 
{
  drawLine(x, y, x+w-1, y, color);
}
		
/**
 * @brief Draws a vertical line on the screen
 * @param x - x coordinate of the start
 * @param y - y coordinate of the start
 * @param w - the height of the line
 * @param color - the color to set
 */
void DISPLAY_SPI::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) 
{
  drawLine(x, y, x, y+h-1, color);
}

/**
 * @brief Draws a vertical line on the screen
 * @param x - x coordinate of the start
 * @param y - y coordinate of the start
 * @param x1 - x coordinate of the end
 * @param y1 - y coordinate of the end
 * @param color - the color to set
 */
void DISPLAY_SPI::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) 
{
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0<=x1; x0++) {
    if (steep) {
      draw_pixel(y0, x0, color);
    } else {
      draw_pixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

/**
 * @brief Fill area from x to x+w, y to y+h
 * @param x - x Coordinate
 * @param y - y Coordinate
 * @param w - width
 * @param h - height
 * @param color - color
 */
void DISPLAY_SPI::fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  for (int16_t i=x; i<x+w; i++) {
    drawFastVLine(i, y, h, color);
  }
}

/**
 * @brief Fills the screen with a color
 * @param color - the color to fill the screen with
 */
void DISPLAY_SPI::fillScreen(uint16_t color) 
{
  fill_rect(0, 0, width, height, color);
}

/**
 * @brief Gets the display height
 * @returns The display height
 */
int16_t DISPLAY_SPI::get_height() const
{
	return height;
}

/**
 * @brief Gets the current display rotation
 * @returns the rotation: 
 * 		0  :  0 degree 
 *		1  :  90 degree
 *		2  :  180 degree
 *		3  :  270 degree
 */
uint8_t DISPLAY_SPI::get_rotation(void) const
{
	return rotation;
}

/**
 * @brief Gets the display width
 * @returns The display width
 */
int16_t DISPLAY_SPI::get_width() const
{
	return width;
}

/**
 * @brief Initializes the display
 */
void DISPLAY_SPI::init()
{
	Logger.Info(F("....Starting SPI display init."));
	pinMode(RS, OUTPUT);
	pinMode(CS, OUTPUT);
	CS_IDLE;
	CD_DATA;
	
	spi_settings = SPISettings(SPI_BUS_FREQUENCY, MSBFIRST, SPI_MODE0);
  spi = new SPIClass(HSPI);
  spi->begin(SCK, -1, SID, CS);
	  // explicitely pass the PINs since we need to avoid the assignment of the MISO pin as 
		// this pin is required elsewhere.
	if(RESET > 0) pinMode(RESET, OUTPUT);
	if(LED > 0)
	{
		pinMode(LED, OUTPUT);
		digitalWrite(LED, LOW);
	}
	reset();

	uint8_t cmd, x, numArgs;
	const uint8_t *addr = initcmd;
	while ((cmd = pgm_read_byte(addr++)) > 0) {
		x = pgm_read_byte(addr++);
		numArgs = x & 0x7F;
		sendCommand(cmd, addr, numArgs);
		addr += numArgs;
		if (x & 0x80)
		delay(150);
	}

	rotation = 0;
	width = TFT_WIDTH;
	height = TFT_HEIGHT;
	toggle_backlight(true);
	Logger.Info(F("....SPI display init complete."));
}

/**
 * @brief Reset the display
 */
void DISPLAY_SPI::reset()
{
	if(RESET > 0)
	{
		digitalWrite(RESET, LOW);
		delay(20);
		digitalWrite(RESET, HIGH);
		delay(120);

		sendCommand(ILI9341_NOP);
	}
	else
	{
		sendCommand(ILI9341_SOFTRESET);
		delay(120);
	}
}

/**
 * @brief Set display rotation
 * @param r - The Rotation to set. 
 * 					 0 - 0 degree
 * 					 1 - 90 degree
 * 					 2 - 180 degree
 * 					 3 - 270 degree
 */
void DISPLAY_SPI::set_rotation(uint8_t r)
{
  rotation = r % 4; // can't be higher than 3
  switch (rotation) {
  case 0:
    r = (MADCTL_MX | MADCTL_BGR); width = TFT_WIDTH; height = TFT_HEIGHT;
    break;
  case 1:
    r = (MADCTL_MV | MADCTL_BGR); width = TFT_HEIGHT; height = TFT_WIDTH;
    break;
  case 2:
    r = (MADCTL_MY | MADCTL_BGR); width = TFT_WIDTH; height = TFT_HEIGHT;
    break;
  case 3:
    r = (MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR); width = TFT_HEIGHT; height = TFT_WIDTH;
    break;
  }
  sendCommand(ILI9341_MADCTL, &r, 1);
}

/**
 * @brief Toggles the backlight on or off if an LED Pin is connected
 * @param state - true to turn the backlight on, false to turn it off. 
 */
void DISPLAY_SPI::toggle_backlight(boolean state)
{
	if(LED >= 0)
	{
		if(state)
		{
			digitalWrite(LED, HIGH);
		}
		else
		{
			digitalWrite(LED, LOW);
		}
	}
}

#pragma endregion

#pragma region protected methods
/**
 * @brief Sets the LCD address window 
 * @param x1 - Upper left x
 * @param y1 - Upper left y
 * @param w - Width
 * @param h - Height
 */
void DISPLAY_SPI::set_addr_window(unsigned int x1, unsigned int y1, unsigned int w, unsigned int h)
{
  static uint16_t old_x1 = 0xffff, old_x2 = 0xffff;
  static uint16_t old_y1 = 0xffff, old_y2 = 0xffff;

  uint16_t x2 = (x1 + w - 1), y2 = (y1 + h - 1);
  if (x1 != old_x1 || x2 != old_x2) {
    writeCommand(ILI9341_CASET); // Column address set
    SPI_WRITE16(x1);
    SPI_WRITE16(x2);
    old_x1 = x1;
    old_x2 = x2;
  }
  if (y1 != old_y1 || y2 != old_y2) {
    writeCommand(ILI9341_PASET); // Row address set
    SPI_WRITE16(y1);
    SPI_WRITE16(y2);
    old_y1 = y1;
    old_y2 = y2;
  }
  writeCommand(ILI9341_RAMWR); // Write to RAM	
}

/** 
 * @brief   Send Command handles complete sending of commands and data
 * @param   commandByte       The Command Byte
 * @param   dataBytes         A pointer to the Data bytes to send
 * @param   numDataBytes      The number of bytes we should send
 */
void DISPLAY_SPI::sendCommand(uint8_t commandByte, const uint8_t *dataBytes, uint8_t numDataBytes) 
{
  SPI_BEGIN_TRANSACTION();
  if (CS >= 0)
    SPI_CS_LOW();

  SPI_DC_LOW();          // Command mode
  SPI_WRITE(commandByte); // Send the command byte

  SPI_DC_HIGH();
  for (int i = 0; i < numDataBytes; i++) {
      SPI_WRITE(*dataBytes); // Send the data bytes
      dataBytes++;
  }

  if (CS >= 0)
    SPI_CS_HIGH();
  SPI_END_TRANSACTION();
}

/*!
    @brief  Write a single command byte to the display. Chip-select and
            transaction must have been previously set -- this ONLY sets
            the device to COMMAND mode, issues the byte and then restores
            DATA mode. There is no corresponding explicit writeData()
            function -- just use spiWrite().
    @param  cmd  8-bit command to write.
*/
void DISPLAY_SPI::writeCommand(uint8_t cmd) 
{
  SPI_DC_LOW();
  SPI_WRITE(cmd);
  SPI_DC_HIGH();
}

#pragma endregion

