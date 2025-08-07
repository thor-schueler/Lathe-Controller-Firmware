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

#include "display_gui.h"
#include "../logging/SerialLogger.h"

#define swap(a, b) { int16_t t = a; a = b; b = t; }

/**
 * @brief Creates a new instance of DISPLAY_GUI
 */
DISPLAY_GUI::DISPLAY_GUI()
{
	text_bgcolor = 0xF800; 	//default red
	text_color = 0x07E0; 	//default green
	draw_color = 0xF800; 	//default red
	text_size = 1;
	text_mode = 0;
}

#pragma region public methods
/**
 * @brief Draw a bit map
 * @param x - x coordinate of upper left
 * @param y - y coordinate of upper left
 * @param sx - width
 * @param sy - height
 * @param data - bitmap data
 * @param scale - 0 to use direct mapping, 1 to scale bitmap to size
 */
void DISPLAY_GUI::draw_bit_map(int16_t x, int16_t y, int16_t sx, int16_t sy, const uint16_t *data, int16_t scale)
{
	int16_t color;
	set_addr_window(x, y, x + sx*scale - 1, y + sy*scale - 1); 
	if(1 == scale)
	{

		push_color_table((uint16_t *)data, sx * sy, 1, 0);
	}
	else 
	{
		for (int16_t row = 0; row < sy; row++) 
		{
			for (int16_t col = 0; col < sx; col++) 
			{
				color = *(data + (row*sx + col)*1); //pgm_read_word(data + (row*sx + col)*1);
				fill_rect(x+col*scale, y+row*scale, scale, scale, color);
			}
		}
	}
}

/** 
 * @brief Draw a circle using the current color
 * @param x - x of center
 * @param y - y of center
 * @param radius - radius of the circle
 */
void DISPLAY_GUI::draw_circle(int16_t x, int16_t y, int16_t radius)
{
	int16_t f = 1 - radius;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * radius;
	int16_t x1= 0;
	int16_t y1= radius;

	draw_pixel(x, y+radius);
 	draw_pixel(x, y-radius);
	draw_pixel(x+radius, y);
	draw_pixel(x-radius, y);

	while (x1<y1) 
	{
    	if (f >= 0) 
		{
      		y1--;
      		ddF_y += 2;
      		f += ddF_y;
    	}
    	x1++;
    	ddF_x += 2;
    	f += ddF_x;
  
		draw_pixel(x + x1, y + y1);
    	draw_pixel(x - x1, y + y1);
		draw_pixel(x + x1, y - y1);
		draw_pixel(x - x1, y - y1);
		draw_pixel(x + y1, y + x1);
		draw_pixel(x - y1, y + x1);
		draw_pixel(x + y1, y - x1);
		draw_pixel(x - y1, y - x1);
 	}
}

/**
 * @brief Draw a line from (x1,y1) to (x2,y2) using the current color
 * @param x1 - x1 coordinate
 * @param y1 - y1 coordinate
 * @param x2 - x2 coordinate
 * @param y2 - y2 coordinate
 */
void DISPLAY_GUI::draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
	int16_t steep = abs(y2 - y1) > abs(x2 - x1);
  	if (steep) 
	{
    	swap(x1, y1);
    	swap(x2, y2);
	}
	if (x1 > x2) 
	{
    	swap(x1, x2);
    	swap(y1, y2);
  	}
	
  	int16_t dx, dy;
  	dx = x2 - x1;
  	dy = abs(y2 - y1);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y1 < y2) 
	{
    	ystep = 1;
  	} 
	else 
	{
    	ystep = -1;
	}

	for (; x1<=x2; x1++) 
	{
    	if (steep) 
		{
      		draw_pixel(y1, x1);
    	} 
		else 
		{
      		draw_pixel(x1, y1);
    	}
    	err -= dy;
    	if (err < 0) 
		{
			y1 += ystep;
			err += dx;
    	}
  	}
}

/** 
 * @brief draw a pixel point usingthe current color
 * @param x - the x coordinate
 * @param y - the y coordinate
 */
void DISPLAY_GUI::draw_pixel(int16_t x, int16_t y)
{
	draw_pixel(x, y, draw_color);
}

/**
 * @brief Draw a rectangle using the current color
 * @param x1 - Upper left corner x coordinate
 * @param y1 - Upper left corner y coordinate
 * @param x2 - Lower right corner x coordinate
 * @param y2 - Lower right corner y coordinate
 */
void DISPLAY_GUI::draw_rectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{ 
	int16_t w = x2 - x1 + 1, h = y2 - y1 + 1;
		//
		// coordinates are zero based
		//
	if (w < 0) 
	{ 
		x1 = x2; 
		w = -w + 2; 
		x2 = x1 + w -1;
	}
	if (h < 0) 
	{ 
		y1 = y2; 
		h = -h + 2; 
		y2 = y1 + h -1;
	}
	draw_fast_hline(x1, y1, w);
  	draw_fast_hline(x1, y2, w);
	draw_fast_vline(x1, y1, h);
	draw_fast_vline(x2, y1, h);
}

/**
 * @brief Draw a rectangle with rounded corners using the current color
 * @param x1 - Upper left corner x coordinate
 * @param y1 - Upper left corner y coordinate
 * @param x2 - Lower right corner x coordinate
 * @param y2 - Lower right corner y coordinate
 * @param radius - coner radius
 */
void DISPLAY_GUI::draw_round_rectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t radius)
{
	int w = x2 - x1 + 1, h = y2 - y1 + 1;
	if (w < 0) 
	{ 
		x1 = x2; 
		w = -w; 
	}
 	if (h < 0) 
	{ 
		y1 = y2; 
		h = -h; 
	}
	draw_fast_hline(x1+radius, y1, w-2*radius); 
	draw_fast_hline(x1+radius, y1+h-1, w-2*radius); 
	draw_fast_vline(x1, y1+radius, h-2*radius); 
  	draw_fast_vline(x1+w-1, y1+radius, h-2*radius);
	draw_circle_helper(x1+radius, y1+radius, radius, 1);
	draw_circle_helper(x1+w-radius-1, y1+radius, radius, 2);
	draw_circle_helper(x1+w-radius-1, y1+h-radius-1, radius, 4);
	draw_circle_helper(x1+radius, y1+h-radius-1, radius, 8);
}

/** 
 * @brief Draw a triangle using the current color
 * @param x0 - first corner x
 * @param y0 - first corner y
 * @param x1 - second corner x
 * @param y1 - second corner y
 * @param x2 - third corner x
 * @param y2 - thrid corner y
 */ 
void DISPLAY_GUI::draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,int16_t x2, int16_t y2)
{
	draw_line(x0, y0, x1, y1);
	draw_line(x1, y1, x2, y2);
  	draw_line(x2, y2, x0, y0);
}

/** 
 * @brief Fill a circle using the current color
 * @param x - x of center
 * @param y - y of center
 * @param radius - radius of the circle
 */
void DISPLAY_GUI::fill_circle(int16_t x, int16_t y, int16_t radius)
{
	draw_fast_vline(x, y-radius, 2*radius+1);
	fill_circle_helper(x, y, radius, 3, 0);
}

/**
 * @brief Fill rectangle with current color
 * @param x1 - Upper left corner x coordinate
 * @param y1 - Upper left corner y coordinate
 * @param x2 - Lower right corner x coordinate
 * @param y2 - Lower right corner y coordinate
 */
void DISPLAY_GUI::fill_rectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
	int w = x2 - x1 + 1, h = y2 - y1 + 1;
   	if (w < 0) 
	{ 
		x1 = x2; 
		w = -w; 
	}
	if (h < 0) 
	{ 
		y1 = y2; 
		h = -h; 
	}
	fill_rect(x1, y1, w, h, draw_color);
}

/**
 * @brief Fill the full screen with color
 * @param color 565 packed color value
 */
void DISPLAY_GUI::fill_screen(uint16_t color)
{
	fill_rect(0, 0, get_width(), get_height(), color);
}

/**
 * @brief Draw a filled rectangle with rounded corners using the current color
 * @param x1 - Upper left corner x coordinate
 * @param y1 - Upper left corner y coordinate
 * @param x2 - Lower right corner x coordinate
 * @param y2 - Lower right corner y coordinate
 * @param radius - corner radius
 */	
void DISPLAY_GUI::fill_round_rectangle(int16_t x1, int16_t y1, int16_t x2,int16_t y2, int16_t radius)
{
	int w = x2 - x1 + 1, h = y2 - y1 + 1;
	if (w < 0) 
	{ 
		x1 = x2; 
		w = -w; 
	}
	if (h < 0) 
	{ 
		y1 = y2; 
		h = -h; 
	}
	fill_rect(x1+radius, y1, w-2*radius, h, draw_color);
	fill_circle_helper(x1+w-radius-1, y1+radius, radius, 1, h-2*radius-1);
	fill_circle_helper(x1+radius, y1+radius, radius, 2, h-2*radius-1);	
}

/**
 * @brief Fill the full screen with color
 * @param r Color R value
 * @param g Color G value
 * @param b Color B value
 */
void DISPLAY_GUI::fill_screen(uint8_t r, uint8_t g, uint8_t b)
{
	uint16_t color = RGB_to_565(r, g, b);
	fill_rect(0, 0, get_width(), get_height(), color);
}

/**
 * @brief Fill a triangle using the current corner
 * @param x0 - first corner x
 * @param y0 - first corner y
 * @param x1 - second corner x
 * @param y1 - second corner y
 * @param x2 - third corner x
 * @param y2 - thrid corner y
 */ 
void DISPLAY_GUI::fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,int16_t x2, int16_t y2)
{
	int16_t a, b, y, last;
  	if (y0 > y1) 
	{
    	swap(y0, y1); 
		swap(x0, x1);
  	}
  	if (y1 > y2) 
	{
    	swap(y2, y1); 
		swap(x2, x1);
  	}
  	if (y0 > y1) 
	{
    	swap(y0, y1); 
		swap(x0, x1);
  	}

	if(y0 == y2) 
	{ 
    	a = b = x0;
    	if(x1 < a)
    	{
			a = x1;
    	}
    	else if(x1 > b)
    	{
			b = x1;
    	}
    	if(x2 < a)
    	{
			a = x2;
    	}
    	else if(x2 > b)
    	{
			b = x2;
    	}
    	draw_fast_hline(a, y0, b-a+1);
    	return;
	}
  	int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0, dx12 = x2 - x1, dy12 = y2 - y1;
	int32_t sa = 0, sb = 0;
	if(y1 == y2)
	{
		last = y1; 
	}
  	else
  	{
		last = y1-1; 
  	}

  	for(y=y0; y<=last; y++) 
	{
    	a   = x0 + sa / dy01;
    	b   = x0 + sb / dy02;
    	sa += dx01;
    	sb += dx02;
    	if(a > b)
    	{
			swap(a,b);
    	}
    	draw_fast_hline(a, y, b-a+1);
	}
	sa = dx12 * (y - y1);
	sb = dx02 * (y - y0);
	for(; y<=y2; y++) 
	{
    	a   = x1 + sa / dy12;
    	b   = x0 + sb / dy02;
    	sa += dx12;
    	sb += dx02;
    	if(a > b)
    	{
			swap(a,b);
    	}
		draw_fast_hline(a, y, b-a+1);
	}
}

/** 
 * @brief Get lcd height
 */ 
int16_t DISPLAY_GUI::get_display_height(void) const
{
	return get_height();
}

/**
 * @brief Get lcd width
 */
int16_t DISPLAY_GUI::get_display_width(void) const
{
	return get_width();
}

/** 
 * @brief get draw color
 * @returns the current draw color
 */ 
uint16_t DISPLAY_GUI::get_draw_color() const
{
	return draw_color;
}

/** 
 * @brief Set 16bits draw color
 * @param color - Color to set
 */
void DISPLAY_GUI::set_draw_color(uint16_t color)
{
	draw_color = color;
}

/** 
 * @brief Get text background color
 * @returns Text background color
 */
uint16_t DISPLAY_GUI::get_text_back_color(void) const
{
	return text_bgcolor;
}

/**
 * @brief Get text colour
 * @returns Text color
 */
uint16_t DISPLAY_GUI::get_text_color(void) const
{
	return text_color;
}

/**
 * @brief Get text mode
 * @returns Text mode
 */
boolean DISPLAY_GUI::get_text_mode(void) const
{
	return text_mode;
}

/** 
 * @brief Get text size
 * @returns Text size
 */
uint8_t DISPLAY_GUI::get_text_size(void) const
{
	return text_size;
}

/**
 * @brief Get text x coordinate
 * @returns The x coordinate of the text cursor
 */
int16_t DISPLAY_GUI::get_text_X_cursor(void) const
{
	return text_x;
}

/**
 * @brief Get text y coordinate
 * @returns The y coordinate of the text cursor
 */
int16_t DISPLAY_GUI::get_text_Y_cursor(void) const
{
	return text_y;
}

/** 
 * @brief Print integer number on the display
 * @param num - the value to print
 * @param x - the X coordinate of the cursor
 * @param y - the Y coordinate of the cursor
 * @param length - the length
 * @param filler - the filler character to use to achieve length
 * @param system - (dec, hexadec, oct, etc....)
 */
void DISPLAY_GUI::print_number_int(long num, int16_t x, int16_t y, int16_t length, uint8_t filler, int16_t system)
{
	uint8_t st[27] = {0};
	uint8_t *p = st+26;
	boolean flag = false;
	int16_t len = 0,nlen = 0,left_len = 0,i = 0;
	*p = '\0';
	if(0 == num)
	{
		*(--p) = '0';
		len = 1;
	}
	else
	{
		if(num < 0)
		{
			num = -num;
			flag = true;
		}		
	}
	while((num > 0) && (len < 10))
	{
		if(num%system > 9)
		{
			*(--p) = 'A' + (num%system-10);
		}
		else
		{
			*(--p) = '0' + num%system;
		}
		num = num/system;
		len++;
	}
	if(flag)
	{
		*(--p) = '-';
	}
	if(length > (len + flag + 1))
	{
		if(length > sizeof(st))
		{
			nlen = sizeof(st) - len - flag - 1;
		}
		else
		{
			nlen = length - len - flag - 1;
		}
		for(i = 0;i< nlen;i++)
		{
			*(--p) = filler;
		}
		left_len = sizeof(st) - nlen - len - flag - 1;
	}	
	else
	{
		left_len = sizeof(st) - len - flag - 1;
	}
	for(i = 0; i < (sizeof(st) - left_len);i++)
	{
		st[i] = st[left_len + i];
	}
	st[i] = '\0';
	print(st, x, y);
}

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
void DISPLAY_GUI::print_number_float(double num, uint8_t dec, int16_t x, int16_t y, uint8_t divider, int16_t length, uint8_t filler)
{
	uint8_t st[27] = {0};
	uint8_t * p = st;
	boolean flag = false;
	int16_t i = 0;
	if(dec<1)
	{
		dec=1;
	}
	else if(dec>5)
	{
		dec=5;
	}
	if(num<0)
	{
		flag = true;
	}
	dtostrf(num, length, dec, (char *)st);
	if(divider != '.')
	{
		while(i < sizeof(st))
		{
			if('.' == *(p+i))
			{
				*(p+i) = divider;
			}
			i++;
		}	
	}
	if(filler != ' ')
	{
		if(flag)
		{
			*p = '-';
			i = 1;
			while(i < sizeof(st))
			{
				if((*(p+i) == ' ') || (*(p+i) == '-'))
				{
					*(p+i) = filler;
				}
				i++;
			}
		}
		else
		{
			i = 0;
			while(i < sizeof(st))
			{
				if(' ' == *(p+i))
				{
					*(p+i) = filler;
				}
			}
		}
	}
	print(st, x, y);
}

/** 
 * @brief Print a string to the display
 * @param st - the String to print
 * @param x - the x coordinate for the cursor
 * @param y - the y coordinate for the cursor
 * @param xo - x origin of the print area
 * @param yo - y origin of the print area
 */
void DISPLAY_GUI::print_string(const uint8_t *st, int16_t x, int16_t y, int16_t xo, int16_t yo)
{
	print((uint8_t *)st, x, y, xo, yo);
}

/** 
 * @brief Print a string to the display
 * @param st - the String to print
 * @param x - the x coordinate for the cursor
 * @param y - the y coordinate for the cursor
 * @param xo - x origin of the print area
 * @param yo - y origin of the print area
 */
void DISPLAY_GUI::print_string(uint8_t *st, int16_t x, int16_t y, int16_t xo, int16_t yo)
{
	print(st, x, y, xo, yo);
}

/** 
 * @brief Print a string to the display
 * @param st - the String to print
 * @param x - the x coordinate for the cursor
 * @param y - the y coordinate for the cursor
 * @param xo - x origin of the print area
 * @param yo - y origin of the print area
 */
void DISPLAY_GUI::print_string(String st, int16_t x, int16_t y, int16_t xo, int16_t yo)
{
	print((uint8_t *)(st.c_str()), x, y, xo, yo);
}

/**
 * @brief Read color data for pixel(x,y)
 * @param x - X coordinate
 * @param y - Y coordinate
 * @returns The color value at the coordinate in packed format
 */
uint16_t DISPLAY_GUI::read_pixel(int16_t x, int16_t y)
{
	uint16_t colour;
	read_GRAM(x, y, &colour, 1, 1);
	return colour;
}

/** 
 * @brief Set 8bits r,g,b color
 * @param r - Red color value
 * @param g - Green color value
 * @param b - Blue color value
 */
void DISPLAY_GUI::set_draw_color(uint8_t r, uint8_t g, uint8_t b)
{
	draw_color = RGB_to_565(r, g, b);
}

/**
 * @brief Set text background color with 16bits color
 * @param color - 16bit packed color value
 */
void DISPLAY_GUI::set_text_back_color(uint16_t color)
{
	text_bgcolor = color;	
}

/** 
 * @brief Set text background color with 8bits r,g,b
 * @param r - R value
 * @param g - G value
 * @param b - B value
 */
void DISPLAY_GUI::set_text_back_color(uint8_t r, uint8_t g, uint8_t b)
{
	text_bgcolor = RGB_to_565(r, g, b);
}

/**
 * @brief Set text color with 16bit packed color
 * @param color - color in packed format
 */
void DISPLAY_GUI::set_text_color(uint16_t color)
{
	text_color = color;
}

/** 
 * @brief Set text colour with 8bits r,g,b
 * @param r - R value
 * @param g - G value
 * @param b - B value
 */
void DISPLAY_GUI::set_text_color(uint8_t r, uint8_t g, uint8_t b)
{
	text_color = RGB_to_565(r, g, b);
}

/** 
 * @brief Set the text cursor coordinate
 * @param x - x coordinate
 * @param y - y coordinate
 */
void DISPLAY_GUI::set_text_cursor(int16_t x, int16_t y)
{
	text_x = x;
	text_y = y;
}

/** 
 * @brief Set text mode
 * @param mode - the text mode
 */
void DISPLAY_GUI::set_text_mode(boolean mode)
{
	text_mode = mode;
}

/** 
 * @brief Set text size
 * @param s - size
 */
void DISPLAY_GUI::set_text_size(uint8_t s)
{
	text_size = s;
}
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
void DISPLAY_GUI::draw_char(int16_t x, int16_t y, uint8_t c, uint16_t color, uint16_t bg, uint8_t size, boolean mode)
{
	if((x >= get_width()) || (y >= get_height()) || ((x + 6 * size - 1) < 0) || ((y + 8 * size - 1) < 0))
	{
    	return;
	}		
  	if(c >= 176)
  	{
		c++; 
  	}
	for (int8_t i=0; i<6; i++) 
	{
    	uint8_t line;
    	if (i == 5)
    	{
      		line = 0x0;
    	}
    	else
    	{
      		line = pgm_read_byte(lcd_font+(c*5)+i);
    	}
    	for (int8_t j = 0; j<8; j++) 
		{
      		if (line & 0x1) 
			{
        		if (size == 1)
        		{
        			draw_pixel(x+i, y+j, color);
        		}
        		else 
				{  
					fill_rect(x+(i*size), y+(j*size), size, size, color);
        		}
        	} 
			else if (bg != color) 				
			{
				if(!mode)
				{
	        		if (size == 1) 
	        		{
	        			draw_pixel(x+i, y+j, bg);
	        		}
	        		else 
					{  
						fill_rect(x+i*size, y+j*size, size, size, bg);
					}
				}
			}
      		line >>= 1;
    	}
    }
}

/** 
 * @brief Draw a circular bead
 * @param x0 - starting point x coordinate
 * @param yo - starting point y coordinate
 * @param radius - radius of the arc
 * @param cornername - determines the direction of the arc
 */
void DISPLAY_GUI::draw_circle_helper(int16_t x0, int16_t y0, int16_t radius, uint8_t cornername)
{
	int16_t f     = 1 - radius;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * radius;
	int16_t x     = 0;
	int16_t y     = radius;
	while (x<y) 
	{
    	if (f >= 0) 
		{
      		y--;
      		ddF_y += 2;
      		f += ddF_y;
    	}
	    x++;
	    ddF_x += 2;
	    f += ddF_x;
	    if (cornername & 0x4) 
		{
			draw_pixel(x0 + x, y0 + y);
			draw_pixel(x0 + y, y0 + x);
	    } 
	    if (cornername & 0x2) 
		{
			draw_pixel(x0 + x, y0 - y);
			draw_pixel(x0 + y, y0 - x);
	    }
	    if (cornername & 0x8) 
		{
			draw_pixel(x0 - y, y0 + x);
			draw_pixel(x0 - x, y0 + y);
	    }
	    if (cornername & 0x1)
		{
			draw_pixel(x0 - y, y0 - x);
	 		draw_pixel(x0 - x, y0 - y);
	    }
  	}
}

/**
 * @brief Draw a vertical line
 * @param x - x coordinate of starting point
 * @param y - y coordinate of starting point
 * @param height - height of the line 
 */ 
void DISPLAY_GUI::draw_fast_vline(int16_t x, int16_t y, int16_t h)
{
	fill_rect(x, y, 1, h, draw_color);
}

/**
 * @brief Draw a horixontal line
 * @param x - x coordinate of starting point
 * @param y - y coordinate of starting point
 * @param w - width of the line 
 */ 
void DISPLAY_GUI::draw_fast_hline(int16_t x, int16_t y, int16_t w)
{
	fill_rect(x, y, w, 1, draw_color);
}

/**
 * @brief Fill a semi-circle with current color
 * @param x0 - starting point x coordinate
 * @param yo - starting point y coordinate
 * @param r - radius of the arc
 * @param cornername - determines the direction of the arc
 * @param delta - mystery offset
 */
void DISPLAY_GUI::fill_circle_helper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta)
{
	int16_t f     = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x     = 0;
	int16_t y     = r;

	while (x<y) 
	{
    	if (f >= 0) 
		{
      		y--;
      		ddF_y += 2;
      		f += ddF_y;
    	}
    	x++;
    	ddF_x += 2;
    	f += ddF_x;

    	if (cornername & 0x1) 
		{
      		draw_fast_vline(x0+x, y0-y, 2*y+1+delta);
      		draw_fast_vline(x0+y, y0-x, 2*x+1+delta);
    	}
    	if (cornername & 0x2) 
		{
      		draw_fast_vline(x0-x, y0-y, 2*y+1+delta);
      		draw_fast_vline(x0-y, y0-x, 2*x+1+delta);
    	}
  	}
}

/**
 * @brief Print string
 * @param st - the string to print
 * @param x - the x coordinate
 * @param y - the y coordinate
 * @param xo - x origin of the print area
 * @param yo - y origin of the print area
 */
size_t DISPLAY_GUI::print(uint8_t *st, int16_t x, int16_t y, int16_t xo, int16_t yo)
{
	int16_t pos;
	uint16_t len;
	const char * p = (const char *)st;
	size_t n = 0;
	if (x == ALIGN_CENTER || x == ALIGN_RIGHT) 
	{
		len = strlen((const char *)st) * 6 * text_size;		
		pos = (get_display_width() - xo - len); 
		if (x == ALIGN_CENTER)
		{
			x = xo + pos/2;
		}
		else
		{
			x = xo + pos - 1;
		}
	}
    set_text_cursor(x + xo, y + yo);
	while(1)
	{
		unsigned char ch = *(p++);
		if(ch == 0)
		{
			break;
		}
		if(write(ch))
		{
			n++;
			if(ch=='\n') text_x = xo;
		}
		else
		{
			break;
		}
	}
	set_text_cursor(text_x - xo, text_y-yo);
	return n;
}

/** 
 * @brief Write a char to the display
 * @param c - char to write
 */
size_t DISPLAY_GUI::write(uint8_t c) 
{
	if (c == '\n') 
	{
    	text_y += text_size*8;
    	text_x  = 0;
 	} 
	else if(c == '\r')
	{
	}
	else 
	{
    	draw_char(text_x, text_y, c, text_color, text_bgcolor, text_size,text_mode);
    	text_x += text_size*6;		
    }	
  	return 1;	
}
#pragma endregion
