
// Copyright (c) Thor Schueler. All rights reserved.
// SPDX-License-Identifier: MIT
// IMPORTANT: LIBRARY MUST BE SPECIFICALLY CONFIGURED FOR EITHER TFT SHIELD
// OR BREAKOUT BOARD USAGE.

#include <SPI.h>
#include "pins_arduino.h"
#include "wiring_private.h"
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
	pinMode(CS, OUTPUT);	  // Enable outputs
	pinMode(RS, OUTPUT);
	pinMode(RESET, OUTPUT);
	if(LED >= 0)
	{
		pinMode(LED, OUTPUT);
		digitalWrite(LED, HIGH);
	}
	digitalWrite(RESET, HIGH);

	spi = new SPIClass(HSPI);
  	spi->begin();
	spi->setFrequency(20000000);
  	spi->setBitOrder(MSBFIRST);
	spi->setDataMode(SPI_MODE0);

	xoffset = 0;
	yoffset = 0;
	rotation = 0;
	width = WIDTH;
	height = HEIGHT;
	setWriteDir();
	CS_IDLE;
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
 * @param image - array to image containing 3 6-bit color values per pixel
 * @param size - the number of elements in the image (should be wxhx3)
 */
void DISPLAY_SPI::draw_background(const unsigned char* image, size_t size)
{
	set_addr_window(0, 0, width - 1, height);
	CS_ACTIVE;
	writeCmd8(CC);
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
 * @param image - array to image containing 3 6-bit color values per pixel
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
	writeCmd8(CC);
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
	set_addr_window(x, y, x, y);
	CS_ACTIVE;
	writeCmd8(CC);
	writeData18(color);
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
	if(true)
	{
		buffer = new uint8_t[(size_t)(h*3)];
		uint8_t r = (uint8_t)((((color >> 11) & 0x1F) * 63)/31);
		uint8_t g = (uint8_t)((color >> 5) & 0x3F);
		uint8_t b = (uint8_t)(((color & 0x1F) * 63) / 31);
		for(i=0; i<h*3; i+=3) 
		{
			buffer[i] = r << 2;
			buffer[i+1] = g << 2;
			buffer[i+2] = b << 2;
		}
		set_addr_window(x, y, x + w - 1, y + h);
			// reducing w by one when setting the address window is important. 
			// the frame memory is written x,y x+1,y, ..., x+w-1,y, x,y+1, ... 
			// so if we do not reduce by 1, we have an extra line.... 
		CS_ACTIVE;
		writeCmd8(CC);
		CD_DATA;
		for (i=0; i<w; i++)
		{
			spi->transferBytes(buffer, nullptr, h*3);
		} 
		CS_IDLE;
		delete[] buffer;
	}
	else
	{
		set_addr_window(x, y, x + w - 1, y + h);
		CS_ACTIVE;
		writeCmd8(CC);
		CD_DATA;
		spi->transferBytes(display_buffer, nullptr, WIDTH * HEIGHT * 3);
		CS_IDLE;
	}
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
	reset();
	toggle_backlight(true);
	start_display();
}

/**
 * @brief Inverts the display
 * @param invert - True to invert, false to revert inversion
 */
void DISPLAY_SPI::invert_display(boolean invert)
{
	CS_ACTIVE;
	uint8_t val = VL^invert;
	writeCmd8(val ? 0x21 : 0x20);
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
		writeCmd8(CC);		
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
		writeData18(color);
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
		writeCmd8(CC);		
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
		
			writeData18(color);
	}
    CS_IDLE;
}

/**
 * @brief Reset the display
 */
void DISPLAY_SPI::reset()
{
	CS_IDLE;
	RD_IDLE;
    WR_IDLE;

    digitalWrite(RESET, LOW);
    delay(2);
    digitalWrite(RESET, HIGH);
  
  	CS_ACTIVE;
  	CD_COMMAND;
  	write8(0x00);
  	for(uint8_t i=0; i<3; i++)
  	{
  		WR_STROBE;
  	}
  	CS_IDLE;
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
		    val = ILI9488_MADCTL_MX | ILI9488_MADCTL_MY | ILI9488_MADCTL_BGR ; //0 degree 
		    break;
		case 1:
		    val = ILI9488_MADCTL_MV | ILI9488_MADCTL_MY | ILI9488_MADCTL_BGR ; //90 degree 
		    break;
		case 2:
		    val = ILI9488_MADCTL_ML | ILI9488_MADCTL_BGR; //180 degree 
		    break;
		case 3:
		    val = ILI9488_MADCTL_MX | ILI9488_MADCTL_ML | ILI9488_MADCTL_MV | ILI9488_MADCTL_BGR; //270 degree
		    break;
	}
	writeCmdData8(MD, val); 
 	set_addr_window(0, 0, width, height);
	vert_scroll(0, HEIGHT, 0);
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

/**
 * @brief Scrolls the display vertically
 * @param scroll_area_top - the top of the scroll 
 * @param scroll_area_height - the height of the scroll area
 * @param offset - scroll distance
 */
void DISPLAY_SPI::vert_scroll(int16_t scroll_area_top, int16_t scroll_area_height, int16_t offset)
{
    int16_t bfa;
    int16_t vsp;
	bfa = HEIGHT - scroll_area_top - scroll_area_height; 
    if (offset <= -scroll_area_height || offset >= scroll_area_height)
    {
		offset = 0; //valid scroll
    }
	vsp = scroll_area_top + offset; // vertical start position
    if (offset < 0)
    {
        vsp += scroll_area_height;  //keep in unsigned range
    }
  	uint8_t d[6];           		// for multi-byte parameters
  	d[0] = scroll_area_top >> 8;    // TFA (top fixed area)
  	d[1] = scroll_area_top;
  	d[2] = scroll_area_height >> 8; // VSA (scroll area)
  	d[3] = scroll_area_height;
  	d[4] = bfa >> 8;        		// BFA (bottom fixes area)
  	d[5] = bfa;
	push_command(SC1, d, 6);		// Send the command setting the scroll window

	d[0] = vsp >> 8;        		// Set the scroll start address at the top of the scroll area
  	d[1] = vsp;						// Ending line parameter for the scroll
	push_command(SC2, d, 2);		// Vertical Scroll Start Address command

	if (offset == 0) 
	{
		push_command(0x13, NULL, 0);
	}
}

#pragma endregion

#pragma region protected methods
/**
 * @brief Read graphics RAM data
 * @param x - x Coordinate to start reading from
 * @param y - y Coordinate to start reading from
 * @param block - Pointer to word array to write the data
 * @param w - Width of the area to read
 * @param h - height of the area to read
 * @returns The number of words read
 */
uint32_t DISPLAY_SPI::read_GRAM(int16_t x, int16_t y, uint16_t *block, int16_t w, int16_t h)
{
	uint16_t ret, dummy;
    uint32_t n = w * h;
	uint32_t cnt = 0;
    uint8_t r, g, b, tmp;
    set_addr_window(x, y, x + w - 1, y + h - 1);
    while (n > 0) 
	{
        CS_ACTIVE;
		writeCmd16(RC);
        setReadDir();

		read8(r);
        while (n) 
		{
			if(R24BIT == 1)
			{
        		read8(r);
         		read8(g);
        		read8(b);
            	ret = RGB_to_565(r, g, b);
			}
			else if(R24BIT == 0)
			{
				read16(ret);
			}
            *block++ = ret;
            n--;
			cnt++;
        }
        CS_IDLE;
        setWriteDir();
    }
	return cnt;
}

/**
 * @brief Read graphics RAM data as 565 values
 * @param x - x Coordinate to start reading from
 * @param y - y Coordinate to start reading from
 * @param block - Pointer to word array to write the data
 * @param w - Width of the area to read
 * @param h - height of the area to read
 * @returns The number of values read
 */
uint32_t DISPLAY_SPI::read_GRAM_RGB(int16_t x, int16_t y, uint8_t *block, int16_t w, int16_t h)
{
	uint32_t ret;
    uint32_t n = (uint32_t)w * (uint32_t)h * 3;
	uint32_t cnt = 0;
    uint8_t r;

    set_addr_window(x, y, x+w-1, y+h-1);
	CS_ACTIVE;
	writeCmd16(0x2E);
    setReadDir();

	r=spi->transfer(0x00);  // first byte just contains some status info... discard...
    if(R24BIT == 1)
	{
		for (uint32_t i = 0; i < n; i++) 
		{ 
			block[i] = (spi->transfer(0x00) & 0x7F) << 1;
			cnt++;
		}
	}
	else
	{
		for (uint32_t i = 0; i < n; i+=3) 
		{ 
			block[i] = (uint8_t)((((ret >> 11) & 0x1F) * 63)/31);
			block[i+1] = (uint8_t)((ret >> 5) & 0x3F);
			block[i+2] = (uint8_t)(((ret & 0x1F) * 63) / 31);
				// this is likely incorrect and needs to be empircally reviewed
				// to make sure that there is no internal mucking up the colors ;)
			cnt+=3;
		}
	}
    CS_IDLE;
    setWriteDir();
	return cnt;
}


/**
 * @brief Read the value from LCD register
 * @param reg - the register to read
 * @param index - the number of words to read
 */
uint16_t DISPLAY_SPI::read_reg(uint16_t reg, int8_t index)
{
	uint16_t ret,high;
    uint8_t low;
	CS_ACTIVE;
    writeCmd16(reg);
    setReadDir();
    delay(1); 
	do 
	{ 
		read16(ret);
	} while (--index >= 0);  
    CS_IDLE;
    setWriteDir();
    return ret;
}

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
	push_command(XC, x_buf, 4);
		// Send Column Address Set Command 
		// This command is used to define the area of the frame memory that the MCU can access. This command makes no change on 
		// the other driver status. The values of SC [15:0] (first and second data bytes) and EC [15:0] (third and fourth data bytes) 
		// are referred when RAMWR command is applied. Each value 
		// represents one column line in the Frame Memory.
	push_command(YC, y_buf, 4);
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
	reset();
	delay(200);
		
	static const uint8_t ILI9488_IPF[] PROGMEM ={0x3A,1,0x66};
	init_table8(ILI9488_IPF, sizeof(ILI9488_IPF));

	XC=ILI9488_COLADDRSET,YC=ILI9488_PAGEADDRSET,CC=ILI9488_MEMORYWRITE,RC=HX8357_RAMRD,SC1=0x33,SC2=0x37,MD=ILI9488_MADCTL,VL=0,R24BIT=1;
	static const uint8_t ILI9488_regValues[] PROGMEM = 
	{
		0xF7, 4, 0xA9, 0x51, 0x2C, 0x82,	// Send Adjust Control 3 command 
											// First parameter - constant
											// Second parameter - constant
											// Third parameter - constant
											// Fourth parameter - DSI write DCS command, use loose packet RGB 666
		0xC0, 2, 0x11, 0x09,				// Send Power Control 1 command
											// First parameter - Set the VREG1OUT voltage for positive gamma 1.25 x 3.70 = 4.6250
											// Second parameter - Set the VREG2OUT voltage for negative gammas -1.25 x 3.30 = -4.1250 
		0xC1, 1, 0x41,						// Send Power Control 2  command
											// First parameter - Set the factor used in the step-up circuits.
                          					//    DDVDH = VCI x 2
                          					//    DDVDL = -(VCI x 2)
                          					//    VCL = -VCI
                          					//    VGH = VCI x 6
                          					//    VGL = -VCI x 4
		0xC5, 3, 0x00, 0x0A, 0x80,			// Send VCOM Control command  
											// First parameter - NV memory is not programmed
											// Second parameter - Used to set the factor to generate VCOM voltage from the reference voltage VREG2OUT.  VCOM = -1.75 
											// Third parameter - Select the Vcom value from VCM_REG [7:0] or NV memory. 1: VCOM value from VCM_REG [7:0].
		0xB1, 2, 0xB0, 0x11,				// Send Frame Rate Control (In Normal Mode/Full Colors)
											// First parameter - 
					                        //    Set division ratio for internal clocks when Normal mode: 00 - Fosc
                          					//    Set the frame frequency of full color normal mode: CNT = 17, Frame Rate 60.76 
											// Second parameter - Is used to set 1H (line) period of the Normal mode at the MCU interface: 17 clocks
		0xB4, 1, 0x02,						// Send Display Inversion Control command
											// First Parameter - set the Display Inversion mode: 2 dot inversion
		0xB6, 2, 0x02, 0x22,				// Send Display Function Control command
											// First parameter -
                          					//    0000 0010
                          					//    0           Select the display data path (memory or direct to shift register) when the RGB interface is used. Bypass - Memory
                          					//     0          RCM RGB interface selection (refer to the RGB interface section). DE Mode
                          					//      0         Select the interface to access the GRAM. When RM = 0, the driver will write display data to the GRAM via the system 
                          					//                interface, and the driver will write display data to the GRAM via the RGB interface when RM = 1.
                          					//       0        Select the display operation mode: Internal system clock  
                          					//         00     Set the scan mode in a non-display area: Normal scan
                          					//           10   Determine source/VCOM output in a non-display area in the partial display mode: AGND   
											// Second parameter - 
                          					//     0010 0010  
                          					//     00         Set the direction of scan by the gate driver: G1 -> G480
                          					//       1        Select the shift direction of outputs from the source driver: S960 -> S1        
                          					//        0       Set the gate driver pin arrangement in combination with the GS bit (RB6h) to select the optimal scan mode for the module: G1->G2->G3->G4 ………………… G477->G478->G479->G480 
                          					//          0010  Set the scan cycle when the PTG selects interval scan in a non-display area drive period. The scan cycle is defined 
                          					//                by n frame periods, where n is an odd number from 3 to 31. The polarity of liquid crystal drive voltage from the gate driver is 
                          					//                inverted in the same timing as the interval scan cycle: 5 frames (84ms)
		0xB7, 1, 0xC6,						// Send Entry Mode Set command
											// First parameter - 
                          					//    1100 0110
                          					//    1100        Set the data format when 16bbp (R, G, B) to 18 bbp (R, G, B) is stored in the internal GRAM. See ILI9488 datasheet 
                          					//         0      The ILI9488 driver enters the Deep Standby Mode when the DSTB is set to high (= 1). In the Deep Standby mode, 
                          					//                both internal logic power and SRAM power are turned off, the display data are stored in the Frame Memory, and the 
                          					//                instructions are not saved. Rewrite Frame Memory content and instructions after exiting the Deep Standby Mode.
                          					//          11    Set the output level of the gate driver G1 ~ G480 as follows: Normal display  
                          					//            0   Low voltage detection control: Enable		
		0xBE, 2, 0x00, 0x04,				// Send HS Lanes Control command
											// First parameter - Type 1 
											// Second parameter - ESD protection: on
		0xE9, 1, 0x00,						// Send Set Image Function command
											// First parameter -  Enable 24-bits Data Bus; users can use DB23~DB0 as 24-bits data input: off
		0x36, 1, 0x08,						// Send Memory Access Control command
											// First parameter - see datasheet 
                          					//    0000 1000   
                          					//    0         MY - Row Address Order
                          					//     0        MX - Column Access Order
                          					//      0       MV - Row/Column Exchange
                          					//       0      ML - Vertical Refresh Order
                          					//         1    BGR- RBG-BGR Order
                          					//          0   MH - Horizontal Refresh Order
		0x3A, 1, 0x66,						// Send Interface Pixel Format command
											// First parameter - 
                          					//    0110 0110
                          					//    0110      RGB Interface Format 18bits/pixel
                          					//         0110 MCU Interface Format 18bits/pixel
											// using 0x66 sends data as 3 data bytes per pixel, each byte using the lower 6 bits for color information. 
											// it is also possible for use 0x65 to configure for 565 RGB values. This results in better performance but loss of
											// some color depth. In 565 mode, each pixel gets a 16bit color information (565). This means 30% less data over the buss
											// and 30% less memory requirements. 
		0xE0, 15, 0x00, 0x07, 0x10, 0x09, 0x17, 0x0B, 0x41, 0x89, 0x4B, 0x0A, 0x0C, 0x0E, 0x18, 0x1B, 0x0F,
											// Send PGAMCTRL(Positive Gamma Control) command
											// Parameters 1 - 15 - Set the gray scale voltage to adjust the gamma characteristics of the TFT panel.
		0xE1, 15, 0x00, 0x17, 0x1A, 0x04, 0x0E, 0x06, 0x2F, 0x45, 0x43, 0x02, 0x0A, 0x09, 0x32, 0x36, 0x0F,
											// Send NGAMCTRL(Negative Gamma Control) command 
											// Parameters 1 - 15 - Set the gray scale voltage to adjust the gamma characteristics of the TFT panel.
		0x51, 1, 0xff,						// Send brightness command to fill brightness
		0x11, 0,							// Send Sleep OUT command 	
		0x29, 0								// Send Display On command
	};
	init_table8(ILI9488_regValues, sizeof(ILI9488_regValues));
	set_rotation(rotation); 
	invert_display(false);
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
		writeCmd8(YC+i);    
		writeCmd8(0x02); 
		writeCmd8(XC); 
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
 * @brief Pushes initialization data and commands to the display controller
 * @details This method uses word data. The first word is a command, the second the number of parameters, followed by all the parameters, then next command etc.....
 * @param table - Pointer to table of word data
 * @param size - The number of words in the table 
 */
void DISPLAY_SPI:: init_table16(const void *table, int16_t size)
{
    uint16_t *p = (uint16_t *) table;
    while (size > 0) 
	{
        uint16_t cmd = pgm_read_word(p++);
        uint16_t d = pgm_read_word(p++);
        if (cmd == TFTLCD_DELAY16)
        {
            delay(d);
        }
        else 
		{
			write_cmd_data(cmd, d);
		}
        size -= 2 * sizeof(int16_t);
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

