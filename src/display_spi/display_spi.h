// Copyright (c) Thor Schueler. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _DISPLAY_SPI_H_
#define _DISPLAY_SPI_H_

#include "Arduino.h"
#include <SPI.h>
#include "mcu_spi_magic.h"
#include "../display_gui/display_gui.h"

/** 
 * This program implements the SPI display for the wheel.
 * if you don't need to control the LED pin,you can set it to 3.3V and set the pin definition to -1.
 * other pins can be defined by youself,for example
 * pin usage as follow:
 *                   CS  DC/RS  RESET  SDI/MOSI  MISO 	SCK   LED    VCC     GND    
 * ESP32             15   25     26       13     12		14    3.3V   3.3V    GND
 */

/**
 * @brief Implements the communication with the SPI controller
 */
class DISPLAY_SPI:public DISPLAY_GUI
{
	public:
		/**
		 * @brief Generates a new instance of the DISPLAY_SPI class. 
		 * @details initializes the SPI and LCD pins including CS, RS, RESET 
		 */
		DISPLAY_SPI();

		/**
		 * @brief Pass 8-bit (each) R,G,B, get back 16-bit packed color
		 * @param r - Red color value
		 * @param g - Green color value
		 * @param b - Blue color value
		 * @returns 16-bit packed color value
		 */
		uint16_t RGB_to_565(uint8_t r, uint8_t g, uint8_t b) override;

		/**
		 * @brief draw backgound image on the display
		 * @param image - array to image containing 3 6-bit color values per pixel
		 * @param size - the number of elements in the image (should be wxhx3)
		 */
		void draw_background(const unsigned char* image, size_t size) override;

		/**
		 * @brief Draws a bitmap on the display
		 * @param x - X coordinate of the upper left corner
		 * @param y - Y coordinate of the upper left corner
		 * @param width - Width of the bitmap
		 * @param height - Height of the bitmap
		 * @param BMP - Pointer to the bitmap data
		 * @param mode - Bitmap mode (normal or inverse)
		 */
		void draw_bitmap(uint8_t x,uint8_t y,uint8_t width, uint8_t height, uint8_t *BMP, uint8_t mode);

		/**
		 * @brief draw image on the display
		 * @param image - array to image containing 3 6-bit color values per pixel
		 * @param size - the number of elements in the image (should be wxhx3)
		 * @param x - starting x coordinate
		 * @param y - starting y coordinate
		 * @param w - image width
		 * @param h - image height
		 */
		void draw_image(const unsigned char* image, size_t size, uint16_t x, uint16_t y, uint16_t w, uint16_t h) override;

		/**
		 * @brief Draws a pixel of a certain color at a certain location
		 * @param x - x coordinate of the pixel
		 * @param y - y coordinate of the pixel
		 * @param color - the color of the pixel to set
		 */
		void draw_pixel(int16_t x, int16_t y, uint16_t color) override;

		/**
		 * @brief Fill area from x to x+w, y to y+h
		 * @param x - x Coordinate
		 * @param y - y Coordinate
		 * @param w - width
		 * @param h - height
		 * @param color - color
		 */
		void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;

		/**
		 * @brief Gets teh display height
		 * @returns The display height
		 */
		int16_t get_height(void) const override;

		/**
		 * @brief Gets the current display rotation
		 * @returns the rotation: 
		 * 		0  :  0 degree 
		 *		1  :  90 degree
		*		2  :  180 degree
		*		3  :  270 degree
		*/	
		uint8_t get_rotation(void) const;

		/**
		 * @brief Gets the display width
		 * @returns The display width
		 */
		int16_t get_width(void) const override;

		/**
		 * @brief Initializes the display
		 */
		void init();

		/**
		 * @brief Inverts the display
		 * @param invert - True to invert, false to revert inversion
		 */
		void invert_display(boolean i);

		/**
		 * @brief Push color table for 16 bits to controller
		 * @param block - the color table
		 * @param n - the number of colors in the table
		 * @param first - true to first send an initialization command
		 * @param flags - flags
		 */
		void push_color_table(uint16_t * block, int16_t n, bool first, uint8_t flags) override;

		/**
		 * @brief Push color table for 8 bits to controller
		 * @param block - the color table
		 * @param n - the number of colors in the table
		 * @param first - true to first send an initialization command
		 * @param flags - flags
		 */	
		void push_color_table(uint8_t * block, int16_t n, bool first, uint8_t flags);

		/**
		* @brief Reset the display
		*/
		void reset();

		/**
		 * @brief Set display rotation
		 * @param rotation - The Rotation to set. 
		 * 					 0 - 0 degree
		 * 					 1 - 90 degree
		 * 					 2 - 180 degree
		 * 					 3 - 270 degree
		 */
		void set_rotation(uint8_t r); 

		/**
		 * @brief Toggles the backlight on or off if an LED Pin is connected
		 * @param state - true to turn the backlight on, false to turn it off. 
		 */
		void toggle_backlight(boolean state);

		/**
		 * @brief Scrolls the display vertically
		 * @param scroll_area_top - the top of the scroll 
		 * @param scroll_area_height - the height of the scroll area
		 * @param offset - scroll distance
		 */
		void vert_scroll(int16_t scroll_area_top, int16_t scroll_area_height, int16_t offset);

	protected:
		/**
		 * @brief Read graphics RAM data as 565 values
		 * @param x - x Coordinate to start reading from
		 * @param y - y Coordinate to start reading from
		 * @param block - Pointer to word array to write the data
		 * @param w - Width of the area to read
		 * @param h - height of the area to read
		 * @returns The number of values read
		 */
		uint32_t read_GRAM(int16_t x, int16_t y, uint16_t *block, int16_t w, int16_t h) override;

		/**
		 * @brief Read graphics RAM data as RGB byte values
		 * @param x - x Coordinate to start reading from
		 * @param y - y Coordinate to start reading from
		 * @param block - Pointer to word array to write the data
		 * @param w - Width of the area to read
		 * @param h - height of the area to read
		 * @returns The number of bytes read
		 */
		uint32_t read_GRAM_RGB(int16_t x, int16_t y, uint8_t *block, int16_t w, int16_t h);

		/**
		 * @brief Read the value from LCD register
		 * @param reg - the register to read
		 * @param index - the number of words to read
		 */
		uint16_t read_reg(uint16_t reg, int8_t index);

		/**
		 * @brief Sets the LCD address window 
		 * @param x1 - Upper left x
		 * @param y1 - Upper left y
		 * @param x2 - Lower right x
		 * @param y2 - Lower right y
		 */
		void set_addr_window(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2) override;

		/**
		 * @brief Starts the display, initializes registers
		 */
		void start_display();

		/**
		 * @brief Writes the display buffer contents to the display
		 * @remarks The display buffer is stored in the static display_buffer array
		 */
		void write_display_buffer();

		/**
		 * @brief Writes command and data block to the display controller
		 * @param cmd - The command to write
		 * @param data - The block of data to write
		 * @param data_size - Size of the data block
		 */
		void push_command(uint8_t cmd, uint8_t *data, int8_t data_size);

		/**
		 * @brief Performs a write on the SPI bus.
		 * @param data - data to write
		 */
		void spi_write(uint8_t data);
		
		/**
		 * @brief Read data from the SPI bus
		 * @return the data read from the bus
		 */
		uint8_t spi_read();
		
		/**
		 * @brief Writes a command to the display controller.
		 * @param cmd - Command to write
		 */
		void write_cmd(uint16_t cmd);

		/**
		 * @brief Writes data to the display controller.
		 * @param data - Data to write
		 */		
		void write_data(uint16_t data);

		/**
		 * @brief Writes command and data combination to the display controller.
		 * @param cmd - Command to write
		 * @param data - Data to write
		 */
		void write_cmd_data(uint16_t cmd, uint16_t data);

		/**
		 * @brief Pushes initialization data and commands to the display controller
		 * @details This method uses byte data. The first byte is a command, the second the number of parameters, followed by all the parameters, then next command etc.....
		 * @param table - Pointer to table of byte data
		 * @param size - The number of bytes in the table 
		 */
		void init_table8(const void *table, int16_t size);
		
		/**
		 * @brief Pushes initialization data and commands to the display controller
		 * @details This method uses word data. The first word is a command, the second the number of parameters, followed by all the parameters, then next command etc.....
		 * @param table - Pointer to table of word data
		 * @param size - The number of words in the table 
		 */	
		void init_table16(const void *table, int16_t size);

		uint8_t xoffset,yoffset;
    	uint16_t rotation;
		unsigned int width = WIDTH;
		unsigned int height = HEIGHT;
		uint16_t XC,YC,CC,RC,SC1,SC2,MD,VL,R24BIT;
		SPIClass *spi = NULL;
		volatile uint32_t *spicsPort, *spicdPort, *spimisoPort , *spimosiPort, *spiclkPort;
		uint8_t  spicsPinSet, spicdPinSet  ,spimisoPinSet , spimosiPinSet , spiclkPinSet, spicsPinUnset, spicdPinUnset, spimisoPinUnset,  spimosiPinUnset,spiclkPinUnset;
};
#endif
