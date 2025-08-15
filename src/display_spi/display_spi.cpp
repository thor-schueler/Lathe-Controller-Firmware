
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


static const uint8_t display_buffer[WIDTH * HEIGHT * 3] PROGMEM = {0};

/**
 * @brief Generates a new instance of the DISPLAY_SPI class. 
 * @details initializes the SPI and LCD pins including CS, RS, RESET 
 */
DISPLAY_SPI::DISPLAY_SPI()
{
	//if(RESET > 0) pinMode(RESET, OUTPUT);
	//if(LED > 0)
	//{
	//	pinMode(LED, OUTPUT);
	//		digitalWrite(LED, LOW);
	//}

	//spi = new SPIClass(HSPI);
	//spi->setFrequency(20000000);
	//spi->setBitOrder(MSBFIRST);
	//spi->setDataMode(SPI_MODE0);
	//spi->begin(SCK, -1, SID, CS);
		// explicitely pass the PINs since we need to avoid the assignment of the MISO pin as 
		// this pin is required elsewhere. 

	//pinMode(RS, OUTPUT);
	//rotation = 0;
	//width = WIDTH;
	//height = HEIGHT;
	//setWriteDir();
	//CS_IDLE;
}

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
	set_addr_window(0, 0, width - 1, height);
	CS_ACTIVE;
	writeCmd8(ILI9341_MEMORYWRITE);
	CD_DATA;
	spi->transferBytes(image, nullptr, size);
	CS_IDLE;
}

/**
 * @brief Draws a bitmap on the display
 * @param x - X coordinate of the upper left corner
 * @param y - Y coordinate of the upper left corner
 * @param width - Width of the bitmap
 * @param height - Height of the bitmap
 * @param BMP - Pointer to the bitmap data
 * @param mode - Bitmap mode (normal or inverse)
 */
void DISPLAY_SPI::draw_bitmap(uint8_t x,uint8_t y,uint8_t width, uint8_t height, uint8_t *BMP, uint8_t mode)
{
  uint8_t i,j,k;
  uint8_t tmp;
  for(i=0;i<(height+7)/8;i++)
  {
		for(j=0;j<width;j++)
		{
			if(mode)
			{
				tmp = pgm_read_byte(&BMP[i*width+j]);
			}
			else
			{
				tmp = ~(pgm_read_byte(&BMP[i*width+j]));
			}
			for(k=0;k<8;k++)
			{
				if(tmp&0x01)
				{
					draw_pixel(x+j, y+i*8+k,1);
				}
				else
				{
					draw_pixel(x+j, y+i*8+k,0);
				}
				tmp>>=1;
			}
		}
   } 
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
	set_addr_window(x, y, x+ w - 1, y + h);
	CS_ACTIVE;
	writeCmd8(ILI9341_MEMORYWRITE);
	CD_DATA;
	spi->transferBytes(image, nullptr, size);
	CS_IDLE;
}

/**
 * @brief Draws a pixel of a certain color at a certain location
 * @param x - x coordinate of the pixel
 * @param y - y coordinate of the pixel
 * @param color - the color of the pixel to set
 */
void DISPLAY_SPI::draw_pixel(int16_t x, int16_t y, uint16_t color)
{
	if((x < 0) || (y < 0) || (x > get_width()) || (y > get_height()))
	{
		return;
	}
	set_addr_window(x, y, 1, 1);
	CS_ACTIVE;
	writeCmd8(ILI9341_MEMORYWRITE);
	writeData16(color);
	CS_IDLE;
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
	int16_t end; 
	uint8_t *buffer;
	uint16_t i;
	if (w < 0) 
	{
		w = -w;
		x -= w;
	}                           //+ve w
	end = x + w;
	if (x < 0)
	{
		x = 0;
	}
	if (end > get_width())
	{
		end = get_width();
	}
	w = end - x;
	if (h < 0) 
	{
		h = -h;
		y -= h;
	}                           //+ve h
	end = y + h;
	if (y < 0)
	{
		y = 0;
	}
	if (end > get_height())
	{
		end = get_height();
	}
	h = end - y;

	buffer = new uint8_t[(size_t)(h*2)];
	uint8_t u = (uint8_t)((color >> 8) & 0xFF);
	uint8_t l = (uint8_t)(color & 0xFF);
	for(i=0; i<h*2; i+=2) 
	{
		buffer[i] = u;
		buffer[i+1] = l;
	}
	set_addr_window(x, y, x + w - 1, y + h);
		// reducing w by one when setting the address window is important. 
		// the frame memory is written x,y x+1,y, ..., x+w-1,y, x,y+1, ... 
		// so if we do not reduce by 1, we have an extra line.... 
	CS_ACTIVE;
	writeCmd8(ILI9341_MEMORYWRITE);
	CD_DATA;
	for (i=0; i<w; i++)
	{
		spi->transferBytes(buffer, nullptr, h*2);
	} 
	CS_IDLE;
	delete[] buffer;
}

/**
 * @brief Gets teh display height
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
	pinMode(RS, OUTPUT);
	pinMode(CS, OUTPUT);
	CS_IDLE;
	CD_DATA;
	
	spi_settings = SPISettings(20000000, MSBFIRST, SPI_MODE0);
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
	start_display();

	rotation = 0;
	width = WIDTH;
	height = HEIGHT;
	toggle_backlight(true);
}

/**
 * @brief Inverts the display
 * @param invert - True to invert, false to revert inversion
 */
void DISPLAY_SPI::invert_display(boolean invert)
{
	CS_ACTIVE;
	writeCmd8(invert ? ILI9341_INVERTON : ILI9341_INVERTOFF);
	CS_IDLE;
}

/**
 * @brief Push color table for 16 bits to controller
 * @param block - the color table
 * @param n - the number of colors in the table
 * @param first - true to first send an initialization command
 * @param flags - flags
 */
void DISPLAY_SPI::push_color_table(uint16_t * block, int16_t n, bool first, uint8_t flags)
{
	uint16_t color;
	uint8_t h, l;
	bool isconst = flags & 1;
	CS_ACTIVE;
	if (first) 
	{  
		writeCmd8(ILI9341_MEMORYWRITE);		
	}
	while (n-- > 0) 
	{
		if (isconst) 
		{
			color = pgm_read_word(block++);		
		} 
		else 
		{
			color = (*block++);			

		}
		writeData16(color);
	}
	CS_IDLE;
}

/**
 * @brief Push color table for 8 bits to controller
 * @param block - the color table
 * @param n - the number of colors in the table
 * @param first - true to first send an initialization command
 * @param flags - flags
 */
void DISPLAY_SPI::push_color_table(uint8_t * block, int16_t n, bool first, uint8_t flags)
{
	uint16_t color;
	uint8_t h, l;
	bool isconst = flags & 1;
	bool isbigend = (flags & 2) != 0;
	CS_ACTIVE;
	if (first) 
	{
		writeCmd8(ILI9341_MEMORYWRITE);		
	}
	while (n-- > 0) 
	{
		if (isconst) 
		{
			h = pgm_read_byte(block++);
			l = pgm_read_byte(block++);
		} 
		else 
		{
			h = (*block++);
			l = (*block++);
		}
		color = (isbigend) ? (h << 8 | l) :  (l << 8 | h);
		
		writeData16(color);
	}
	CS_IDLE;
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
	//return;
	rotation = r & 3;           // just perform the operation ourselves on the protected variables
	width = (rotation & 1) ? HEIGHT : WIDTH;
	height = (rotation & 1) ? WIDTH : HEIGHT;
	CS_ACTIVE;

	uint8_t val;
	switch (rotation)
	{
		case 0:
			val = ILI9341_MADCTL_MX | ILI9341_MADCTL_RGB; // 0 degree
			break;
		case 1:
			val = ILI9341_MADCTL_MV | ILI9341_MADCTL_RGB; // 90 degree
			break;
		case 2:
			val = ILI9341_MADCTL_MY | ILI9341_MADCTL_RGB; // 180 degree
			break;
		case 3:
			val = ILI9341_MADCTL_MV | ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_RGB; // 270 degree
			break;
	}
	writeCmdData8(ILI9341_MEMORYACCESS, val);
	set_addr_window(0, 0, width - 1, height - 1);
	CS_IDLE;
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
 * @param x2 - Lower right x
 * @param y2 - Lower right y
 */
void DISPLAY_SPI::set_addr_window(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
{
	CS_ACTIVE;
	uint8_t x_buf[] = {x1>>8,x1,x2>>8,x2};
	uint8_t y_buf[] = {y1>>8,y1,y2>>8,y2};
	push_command(ILI9341_COLADDRSET, x_buf, 4);
		// Send Column Address Set Command 
		// This command is used to define the area of the frame memory that the MCU can access. This command makes no change on 
		// the other driver status. The values of SC [15:0] (first and second data bytes) and EC [15:0] (third and fourth data bytes) 
		// are referred when RAMWR command is applied. Each value 
		// represents one column line in the Frame Memory.
	push_command(ILI9341_PAGEADDRSET, y_buf, 4);
		// Send Page Address Set Command
		// This command is used to define the area of the frame memory that the MCU can access. This command makes no change on 
		// the other driver status. The values of SP [15:0] (first and second data bytes) and EP [15:0] (third and fourth data bytes) 
		// are referred when RAMWR command is applied. Each value 
		// represents one Page line in the Frame Memory.  
	CS_IDLE;		
}

/**
 * @brief Starts the display, initializes registers
 */
void DISPLAY_SPI::start_display()
{
	// ILI9341 initialization sequence
    static const uint8_t ILI9341_regValues[] PROGMEM = {
        0xEF, 3, 0x03, 0x80, 0x02,								// Prepares the controller for extended command access
																// Configures internal voltage or timing parameters
																// May unlock access to other registers used in subsequent commands
        0xCF, 3, 												// Power control B - configures internal power settings related to the step-up circuit and voltage regulator behavior
				0x00, 											// 		Reserved
				0xC1, 											// 		Controls internal step-up or power amplifier gain
				0x30,											// 		Sets Vcore voltage level or regulator output
        0xED, 4, 												// Power on sequence control - vendor-defined command that configures how the internal power circuits ramp up during startup
				0x64,											// 		Power ramp timing or step-up factor 
				0x03, 											// 		Control bits for enabling internal power blocks
				0x12,											// 		Vcore voltage setting or reference
				0x81,											// 		Enable internal regulator or reference voltage
        0xE8, 3,												// Driver Timing Control A - configures internal timing parameters for the display’s power-up and signal synchronization
				0x85, 											//		EQ control or precharge timing for internal power circuits
				0x00, 											//		Reserved
				0x78,											// 		Output timing control—affects gate driver and source driver sync
        0xCB, 5, 												// Power Control A. It configures internal voltage settings and power ramp behavior—critical for ensuring the display’s internal regulators and charge pumps behave predictably during startup.
				0x39, 											//		Vcore control or step-up gain
				0x2C, 											//		Power control tuning
				0x00, 											//		Reserved
				0x34, 											//		VGH/VGL voltage level (gate driver high/low)
				0x02,											//		Internal regulator enable or reference voltage
        0xF7, 1, 												// Pump Ratio Control - configures the ratio of the internal charge pump used to generate higher voltages for gate drivers
				0x20,											//		Sets the charge pump ratio—affects VGH/VGL generation for gate drivers 
		0xEA, 2, 0x00, 0x00,									// Driver Timing Control B - low-level configuration that fine-tunes internal timing parameters for the display’s gate and source drivers
		ILI9341_POWERCONTROL1, 1, 								// Power Control 1 - core voltage configuration commands—it sets the internal voltage regulator that drives the source driver circuitry
				0x23,											//		Sets the internal voltage regulator output (VRH)
        ILI9341_POWERCONTROL2, 1, 								// Power Control 2 - sets the source driver power supply control.
				0x10,											//		SAP[2:0] — Source driver power control
        ILI9341_VCOMCONTROL1, 2, 								// VCOM Control 1 - sets the common electrode voltage levels.
				0x3e, 											//		VCOMH — VCOM high voltage ≈ 4.5V
				0x28,											//		VCOML — VCOM low voltage ≈ -1.5V
        ILI9341_VCOMCONTROL2, 1, 								// VCOM Control 2 - Fine-tunes the VCOM voltage offset.
				0x86,											// 		VCOM offset - empirically tuned for most modules.
        ILI9341_MEMORYACCESS, 1, 								// Memory Access Control (MADCTL)
				0x40,											// 		Bit7 - MY - Row Address Order (flip vertically)
																//		Bit6 - MX -	Column Address Order (flip horizontally)
																//		Bit5 - MV -	Row/Column Exchange (rotate 90°)
																//		Bit3 - BGR - RGB/BGR Order
																//		Bit2 - ML - Vertical Refresh Order
																// 		Bit1 - MH - Horizontal Refresh Order
        ILI9341_VSCRSADD, 1, 0x00,								// Vertical Scroll Offset to 0x00
		ILI9341_PIXELFORMAT, 1, 								// Pixel Format Set
				0x55,											//		0x55 - 16-bit (RGB565), 0x66 - 18-bit (RGB666)
        ILI9341_FRAMECONTROL, 2, 								// Frame Rate Control (Normal Mode)
				0x00, 											//		Division ratio
				0x18,											//		Frame rate control (≈ 70Hz), 0x18 is a balanced value for smooth visuals and low EMI.
        ILI9341_DISPLAYFUNC, 3, 								// Display Function Control - Fine-tunes how pixels are refreshed, Helps reduce flicker and improve visual stability
				0x08, 											//		Scan direction (e.g., left-to-right, top-to-bottom)
				0x82, 											//		LCD drive line configuration
				0x27,											//		Timing control for gate driver and source driver
        0xF2, 1, 0x00,											// Enable 3Gamma Control - 0x00 disables it, allowing manual gamma tuning via 0xE0 and 0xE1.
        ILI9341_GAMMASET, 1, 									// Gamma Set - Selects one of four fixed gamma curves.
				0x01,											//		0x01 - Curve 1 (default), 0x02 - Curve 2, 0x03 - Curve 3, 0x04 - Curve 4
        ILI9341_GMCTRP1, 15, 									// Positive Gamma Correction - Fine-tunes the gamma curve for positive voltages. Higher values → steeper transitions (more contrast), Lower values → flatter transitions (softer gradients)
				0x0F, 											//		V63
				0x31, 0x2B, 0x0C, 0x0E,	0x08,					// 		V1–V5 affect dark tones
				0x4E, 0xF1, 0x37, 0x07, 0x10, 					//		V6–V10 shape midtones
				0x03, 0x0E, 0x09, 0x00,							//		V11–V14 influence highlights and white balance
        ILI9341_GMCTRN1, 15, 									// Negative Gamma Correction - omplements 0xE0 (Positive Gamma Correction) and shapes the tonal curve for darker regions and shadows
				0x00,											//		V63 
				0x0E, 0x14, 0x03, 0x11, 0x07, 					// 		V1–V5: Influence black levels and low-brightness transitions.
				0x31, 0xC1, 0x48, 0x08, 0x0F, 					//		V6–V10: Shape midtone response and contrast.
				0x0C, 0x31, 0x36, 0x0F,							//		V11–V14: Affect highlights and white balance.
        ILI9341_SLEEPOUT, 0,									// Sleep Out - wakes the display from sleep mode.
        TFTLCD_DELAY8, 120,										// Necessary delay for 0x11
        ILI9341_DISPLAYON, 0,									// Display on
		0x00                                   					// End of list
    };

  	uint8_t cmd, x, numArgs;
  	const uint8_t *addr = ILI9341_regValues;
	while ((cmd = pgm_read_byte(addr++)) > 0) 
	{
    	x = pgm_read_byte(addr++);
    	numArgs = x & 0x7F;
		if(x==TFTLCD_DELAY8) delay(numArgs);
		else
		{
    		sendCommand(cmd, addr, numArgs);
    		addr += numArgs;
		}
  	}

	//init_table8(ILI9341_regValues, sizeof(ILI9341_regValues));
	//set_rotation(rotation); 
	//invert_display(false);
	Logger.Info(F("....ILI9341 display startup done."));
}

/**
 * @brief Writes the display buffer contents to the display
 * @remarks The display buffer is stored in the static display_buffer array
 */
void DISPLAY_SPI::write_display_buffer()
{
	uint8_t i, n;	
	CS_ACTIVE;
	for(i=0; i<HEIGHT; i++)  
	{  
		writeCmd8(ILI9341_PAGEADDRSET+i);    
		writeCmd8(0x02); 
		writeCmd8(ILI9341_COLADDRSET); 
		for(n=0; n<WIDTH; n++)
		{
			writeData8(display_buffer[i*WIDTH+n]); 
		}
	} 
	CS_IDLE;
}
#pragma endregion

#pragma region private methods
/**
 * @brief Pushes initialization data and commands to the display controller
 * @details This method uses byte data. The first byte is a command, the second the number of parameters, followed by all the parameters, then next command etc.....
 * @param table - Pointer to table of byte data
 * @param size - The number of bytes in the table 
 */
void DISPLAY_SPI:: init_table8(const void *table, int16_t size)
{
	uint8_t i;
	uint8_t *p = (uint8_t *) table, dat[MAX_REG_NUM]; 
	while (size > 0) 
	{
		uint8_t cmd = pgm_read_byte(p++);
		uint8_t len = pgm_read_byte(p++);
		if (cmd == TFTLCD_DELAY8) 
		{
			delay(len);
			len = 0;
		} 
		else 
		{
			for (i = 0; i < len; i++)
			{
				dat[i] = pgm_read_byte(p++);
			}
			push_command(cmd,dat,len);
		}
		size -= len + 2;
	}
}

/**
 * @brief Writes command and data block to the display controller
 * @param cmd - The command to write
 * @param data - The block of data to write
 * @param data_size - Size of the data block
 */
void DISPLAY_SPI::push_command(uint8_t cmd, uint8_t *data, int8_t data_size)
{
	CS_ACTIVE;
	writeCmd8(cmd);
	while (data_size-- > 0) 
	{
		uint8_t u8 = *data++;
		writeData8(u8); 
	}
	CS_IDLE;
}

/** 
 * @brief   Send Command handles complete sending of commands and data
 * @param   commandByte       The Command Byte
 * @param   dataBytes         A pointer to the Data bytes to send
 * @param   numDataBytes      The number of bytes we should send
 */
void DISPLAY_SPI::sendCommand(uint8_t commandByte, const uint8_t *dataBytes, uint8_t numDataBytes) {
  CS_ACTIVE;
  CD_COMMAND;
  spi_write(commandByte); // Send the command byte

  CD_DATA;
  for (int i = 0; i < numDataBytes; i++) spi_write(pgm_read_byte(dataBytes++));

  CS_IDLE;
}

/**
 * @brief Read data from the SPI bus
 * @return the data read from the bus
 */
uint8_t DISPLAY_SPI::spi_read()
{
	return spi->transfer(0xFF);
}

/**
 * @brief Performs a write on the SPI bus.
 * @param data - data to write
 */
void DISPLAY_SPI::spi_write(uint8_t data)
{
	spi->transfer(data);
}

/**
 * @brief Writes a command to the display controller.
 * @param cmd - Command to write
 */
void DISPLAY_SPI::write_cmd(uint16_t cmd)
{
	CS_ACTIVE;
	writeCmd16(cmd);
	CS_IDLE;
}

/**
 * @brief Writes data to the display controller.
 * @param data - Data to write
 */
void DISPLAY_SPI::write_data(uint16_t data)
{
	CS_ACTIVE;
	writeData16(data);
	CS_IDLE;
}

/**
 * @brief Writes command and data combination to the display controller.
 * @param cmd - Command to write
 * @param data - Data to write
 */
void DISPLAY_SPI::write_cmd_data(uint16_t cmd, uint16_t data)
{
	CS_ACTIVE;
	writeCmdData16(cmd,data);
	CS_IDLE;
}
#pragma endregion

