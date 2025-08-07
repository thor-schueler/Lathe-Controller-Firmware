// Copyright (c) Thor Schueler. All rights reserved.
// SPDX-License-Identifier: MIT
/*
This is the core GUI library for the TFT display, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _DISPLAY_GUI_H_
#define _DISPLAY_GUI_H_

#include "Arduino.h"
#include <pgmspace.h>

#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#define pgm_read_word(addr) (*(const unsigned short *)(addr))
#define ALIGN_LEFT 0
#define ALIGN_RIGHT 9999
#define ALIGN_CENTER 9998

extern const unsigned char lcd_font[] PROGMEM;

class DISPLAY_GUI
{
	public:
		/**
		 * @brief Creates a new instance of the DISPLAY_GUI class
		 */
		DISPLAY_GUI();

		#pragma region Public Virtual methods to be implemented by subclasses
		/// @brief Converts an RGB color specifications to 565 format for the display
		/// @param r - red component value
		/// @param g - green component value
		/// @param b - blue component value
		/// @return integer with the 565 representation of the color. 
		virtual uint16_t RGB_to_565(uint8_t r, uint8_t g, uint8_t b)=0;

		/**
		 * @brief draw backgound image on the display
		 * @param image - array to image containing 3 6-bit color values per pixel
		 * @param size - the number of elements in the image (should be wxhx3)
		 */
		virtual void draw_background(const unsigned char* image, size_t size)=0;

		/**
		 * @brief Draws a pixel of a certain color at a certain location
		 * @param x - x coordinate of the pixel
		 * @param y - y coordinate of the pixel
		 * @param color - the color of the pixel to set
		 */
		virtual void draw_pixel(int16_t x, int16_t y, uint16_t color)=0;

		/**
		 * @brief draw image on the display
		 * @param image - array to image containing 3 6-bit color values per pixel
		 * @param size - the number of elements in the image (should be wxhx3)
		 * @param x - starting x coordinate
		 * @param y - starting y coordinate
		 * @param w - image width
		 * @param h - image height
		 */
		virtual void draw_image(const unsigned char* image, size_t size, uint16_t x, uint16_t y, uint16_t w, uint16_t h)=0;

		/**
		 * @brief Fill area from x to x+w, y to y+h
		 * @param x - x Coordinate
		 * @param y - y Coordinate
		 * @param w - width
		 * @param h - height
		 * @param color - color
		 */
		virtual void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)=0;

		/**
		 * @brief Push color table for 16 bits to controller
		 * @param block - the color table
		 * @param n - the number of colors in the table
		 * @param first - true to first send an initialization command
		 * @param flags - flags
		 */
		virtual void push_color_table(uint16_t * block, int16_t n, bool first, uint8_t flags)=0;

		/**
		 * @brief Gets teh display height
		 * @returns The display height
		 */
		virtual int16_t get_height(void) const=0;

		/**
		 * @brief Gets the display width
		 * @returns The display width
		 */		
		virtual int16_t get_width(void) const=0;
		#pragma endregion 
		
		#pragma region Public Methods
		/**
		 * @brief Draw a bit map
		 * @param x - x coordinate of upper left
		 * @param y - y coordinate of upper left
		 * @param sx - width
		 * @param sy - height
		 * @param data - bitmap data
		 * @param scale - 0 to use direct mapping, 1 to scale bitmap to size
		 */
		void draw_bit_map(int16_t x, int16_t y, int16_t sx, int16_t sy, const uint16_t *data, int16_t scale);

		/** 
		 * @brief Draw a circle using the current color
		 * @param x - x of center
		 * @param y - y of center
		 * @param radius - radius of the circle
		 */		
		void draw_circle(int16_t x, int16_t y, int16_t radius);

		/**
		 * @brief Draw a line from (x1,y1) to (x2,y2) using the current color
		 * @param x1 - x1 coordinate
		 * @param y1 - y1 coordinate
		 * @param x2 - x2 coordinate
		 * @param y2 - y2 coordinate
		 */		
		void draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

		/** 
		 * @brief draw a pixel point usingthe current color
		 * @param x - the x coordinate
		 * @param y - the y coordinate
		 */
		void draw_pixel(int16_t x, int16_t y);

		/**
		 * @brief Draw a rectangle using the current color
		 * @param x1 - Upper left corner x coordinate
		 * @param y1 - Upper left corner y coordinate
		 * @param x2 - Lower right corner x coordinate
		 * @param y2 - Lower right corner y coordinate
		 */
		void draw_rectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

		/**
		 * @brief Draw a rectangle with rounded corners using the current color
		 * @param x1 - Upper left corner x coordinate
		 * @param y1 - Upper left corner y coordinate
		 * @param x2 - Lower right corner x coordinate
		 * @param y2 - Lower right corner y coordinate
		 * @param radius - corner radius
		 */
		void draw_round_rectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t radius);

		/** 
		 * @brief Draw a triangle using the current color
		 * @param x0 - first corner x
		 * @param y0 - first corner y
		 * @param x1 - second corner x
		 * @param y1 - second corner y
		 * @param x2 - third corner x
		 * @param y2 - thrid corner y
		 */ 
		void draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,int16_t x2, int16_t y2);

		/** 
		 * @brief Fill a circle using the current color
		 * @param x - x of center
		 * @param y - y of center
		 * @param radius - radius of the circle
		 */
		void fill_circle(int16_t x, int16_t y, int16_t radius);

		/**
		 * @brief Fill rectangle with current color
		 * @param x1 - Upper left corner x coordinate
		 * @param y1 - Upper left corner y coordinate
		 * @param x2 - Lower right corner x coordinate
		 * @param y2 - Lower right corner y coordinate
		 */		
		void fill_rectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

		/**
		 * @brief Draw a filled rectangle with rounded corners using the current color
		 * @param x1 - Upper left corner x coordinate
		 * @param y1 - Upper left corner y coordinate
		 * @param x2 - Lower right corner x coordinate
		 * @param y2 - Lower right corner y coordinate
		 * @param radius - corner radius
		 */		
		void fill_round_rectangle(int16_t x1, int16_t y1, int16_t x2,int16_t y2, int16_t radius);

		/**
		 * @brief Fill the full screen with color
		 * @param color 565 packed color value
		 */
		void fill_screen(uint16_t color);

		/**
		 * @brief Fill the full screen with color
		 * @param r Color R value
		 * @param g Color G value
		 * @param b Color B value
		 */		
		void fill_screen(uint8_t r, uint8_t g, uint8_t b);

		/**
		 * @brief Fill a triangle using the current corner
		 * @param x0 - first corner x
		 * @param y0 - first corner y
		 * @param x1 - second corner x
		 * @param y1 - second corner y
		 * @param x2 - third corner x
		 * @param y2 - thrid corner y
		 */ 
		void fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,int16_t x2, int16_t y2);

		/** 
		 * @brief Get lcd height
		 */ 
		int16_t get_display_height(void) const; 

		/**
		 * @brief Get lcd width
		 */
		int16_t get_display_width(void) const;

		/** 
		 * @brief get draw color
		 * @returns the current draw color
		 */ 
		uint16_t get_draw_color(void) const;

		/** 
		 * @brief Get text background color
		 * @returns Text background color
		 */	
		uint16_t get_text_back_color(void) const;

		/**
		 * @brief Get text colour
		 * @returns Text color
		 */
		uint16_t get_text_color(void) const;

		/**
		 * @brief Get text mode
		 * @returns Text mode
		 */
		boolean get_text_mode(void) const;

		/** 
		 * @brief Get text size
		 * @returns Text size
		 */
		uint8_t get_text_size(void) const;
		
		/**
		 * @brief Get text x coordinate
		 * @returns The x coordinate of the text cursor
		 */		
		int16_t get_text_X_cursor(void) const;
		
		/**
		 * @brief Get text y coordinate
		 * @returns The y coordinate of the text cursor
		 */		
		int16_t get_text_Y_cursor(void) const;

		/** 
		 * @brief Print integer number on the display
		 * @param num - the value to print
		 * @param x - the X coordinate of the cursor
		 * @param y - the Y coordinate of the cursor
		 * @param length - the length
		 * @param filler - the filler character to use to achieve length
		 * @param system - (dec, hexadec, oct, etc....)
		 */		
		void print_number_int(long num, int16_t x, int16_t y, int16_t length, uint8_t filler, int16_t system);
		
		/** 
		 * @brief Print float number on the display
		 * @param num - the value to print
		 * @param dec - the number of decimal places to print
		 * @param x - the X coordinate of the cursor
		 * @param y - the Y coordinate of the cursor
		 * @param divider - the divider to use
		 * @param length - the length
		 * @param filler - the filler character to use to achieve length
		 */		
		void print_number_float(double num, uint8_t dec, int16_t x, int16_t y, uint8_t divider, int16_t length, uint8_t filler);
		
		/** 
		 * @brief Print a string to the display
		 * @param st - the String to print
		 * @param x - the x coordinate for the cursor
		 * @param y - the y coordinate for the cursor
		 * @param xo - x origin of the print area
		 * @param yo - y origin of the print area
		 */
		void print_string(const uint8_t *st, int16_t x, int16_t y, int16_t xo=0, int16_t yo=0);
		
		/** 
		 * @brief Print a string to the display
		 * @param st - the String to print
		 * @param x - the x coordinate for the cursor
		 * @param y - the y coordinate for the cursor
		 * @param xo - x origin of the print area
		 * @param yo - y origin of the print area
		 */		
		void print_string(uint8_t *st, int16_t x, int16_t y, int16_t xo=0, int16_t yo=0);
		
		/** 
		 * @brief Print a string to the display
		 * @param st - the String to print
		 * @param x - the x coordinate for the cursor
		 * @param y - the y coordinate for the cursor
		 * @param xo - x origin of the print area
		 * @param yo - y origin of the print area
		 */		
		void print_string(String st, int16_t x, int16_t y, int16_t xo=0, int16_t yo=0);

		/**
		 * @brief Read color data for pixel(x,y)
		 * @param x - X coordinate
		 * @param y - Y coordinate
		 * @returns The color value at the coordinate in packed format
		 */
		uint16_t read_pixel(int16_t x, int16_t y);

		/** 
		 * @brief Set 16bits draw color
		 * @param color - Color to set
		 */
		void set_draw_color(uint16_t color);
				
		/** 
		 * @brief Set 8bits r,g,b color
		 * @param r - Red color value
		 * @param g - Green color value
		 * @param b - Blue color value
		 */		
		void set_draw_color(uint8_t r, uint8_t g, uint8_t b);

		/**
		 * @brief Set text background color with 16bits color
		 * @param color - 16bit packed color value
		 */
		void set_text_back_color(uint16_t color); 

		/** 
		 * @brief Set text background color with 8bits r,g,b
		 * @param r - R value
		 * @param g - G value
		 * @param b - B value
		 */
		void set_text_back_color(uint8_t r, uint8_t g, uint8_t b); 

		/**
		 * @brief Set text color with 16bit packed color
		 * @param color - color in packed format
		 */
		void set_text_color(uint16_t color);

		/** 
		 * @brief Set text colour with 8bits r,g,b
		 * @param r - R value
		 * @param g - G value
		 * @param b - B value
		 */
		void set_text_color(uint8_t r, uint8_t g, uint8_t b);

		/** 
		 * @brief Set the text cursor coordinate
		 * @param x - x coordinate
		 * @param y - y coordinate
		 */
		void set_text_cursor(int16_t x, int16_t y);

		/** 
		 * @brief Set text mode
		 * @param mode - the text mode
		 */
		void set_text_mode(boolean mode);

		/** 
		 * @brief Set text size
		 * @param s - size
		 */
		void set_text_size(uint8_t s);
		#pragma endregion
	
	protected:

		#pragma region Protected Virtual methods to be implemented by subclasses
		/**
		 * @brief Sets the LCD address window 
		 * @param x1 - Upper left x
		 * @param y1 - Upper left y
		 * @param x2 - Lower right x
		 * @param y2 - Lower right y
		 */
		virtual void set_addr_window(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)=0;

		/**
		 * @brief Read graphics RAM data
		 * @param x - x Coordinate to start reading from
		 * @param y - y Coordinate to start reading from
		 * @param block - Pointer to word array to write the data
		 * @param w - Width of the area to read
		 * @param h - height of the area to read
		 * @returns The number of words read
		 */		
		virtual uint32_t read_GRAM(int16_t x, int16_t y, uint16_t *block, int16_t w, int16_t h)=0;

		/**
		 * @brief Print string
		 * @param st - the string to print
		 * @param x - the x coordinate
		 * @param y - the y coordinate
		 * @param xo - x origin of the print area
		 * @param yo - y origin of the print area
		 */
		virtual size_t print(uint8_t *st, int16_t x, int16_t y, int16_t xo=0, int16_t yo=0);
		#pragma endregion

		#pragma region protected methods
		/** 
		 * @brief Draw a char on the display
		 * @param x - X position of the cursor
		 * @param y - Y position of the cursor
		 * @param c - the char to draw
		 * @param color  - the packed font color
		 * @param bg - the packed background color
		 * @param size - size of the char
		 * @param mode  - the mode
		 */		
		void draw_char(int16_t x, int16_t y, uint8_t c, uint16_t color,uint16_t bg, uint8_t size, boolean mode);
				
		/** 
		 * @brief Draw a circular bead
		 * @param x0 - starting point x coordinate
		 * @param yo - starting point y coordinate
		 * @param radius - radius of the arc
		 * @param cornername - determines the direction of the arc
		 */
		void draw_circle_helper(int16_t x0, int16_t y0, int16_t radius, uint8_t cornername);

		/**
		 * @brief Draw a vertical line
		 * @param x - x coordinate of starting point
		 * @param y - y coordinate of starting point
		 * @param height - height of the line 
		 */ 		
		void draw_fast_vline(int16_t x, int16_t y, int16_t h);
		
		/**
		 * @brief Draw a horixontal line
		 * @param x - x coordinate of starting point
		 * @param y - y coordinate of starting point
		 * @param w - width of the line 
		 */ 		
		void draw_fast_hline(int16_t x, int16_t y, int16_t w);

		/**
		 * @brief Fill a semi-circle with current color
		 * @param x0 - starting point x coordinate
		 * @param yo - starting point y coordinate
		 * @param r - radius of the arc
		 * @param cornername - determines the direction of the arc
		 * @param delta - mystery offset
		 */
		void fill_circle_helper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername,int16_t delta);

		/** 
		 * @brief Write a char to the display
		 * @param c - char to write
		 */		
		size_t write(uint8_t c);
		#pragma endregion

		///
		/// Member declarations
		///
		int16_t text_x, text_y;
		uint16_t text_color, text_bgcolor,draw_color;
		uint8_t text_size;
		boolean text_mode; //if set,text_bgcolor is invalid
};

#endif

