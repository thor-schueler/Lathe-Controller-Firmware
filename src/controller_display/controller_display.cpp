// Copyright (c) Thor Schueler. All rights reserved.
// SPDX-License-Identifier: MIT
// IMPORTANT: LIBRARY MUST BE SPECIFICALLY CONFIGURED FOR EITHER TFT SHIELD
// OR BREAKOUT BOARD USAGE.

#include <SPI.h>
#include "controller_display.h"
#include "../logging/SerialLogger.h"


/**
 * @brief Generates a new instance of the Controller_Display class. 
 * @details initializes the SPI and LCD pins including CS, RS, RESET 
 */
Controller_Display::Controller_Display()
{
    w_area_x1 = 20;
    w_area_y1 = 20;
    w_area_x2 = 100;
    w_area_y2 = 100;
    w_area_cursor_x = 0;
    w_area_cursor_y = 0;
}

/**
 * @brief Initializes the display
 */
void Controller_Display::init()
{
    //uint32_t buffer_size = (w_area_x2-w_area_x1)*(w_area_y2-w_area_y1)*3/2;
    DISPLAY_SPI::init();
    draw_background(lcars, lcars_size);
    //test();

    //Logger.Info(F("Attempting allocation of screen scrolling memory buffer..."));
    //Logger.Info_f(F("....Largest free block: %d"), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    //buf1 = (uint8_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_8BIT);
    //if(buf1 == nullptr) Logger.Error(F("....Allocation of upper scroll buffer (buf1) did not succeed"));
    //else Logger.Info(F("....Allocation of upper scroll buffer (buf1) successful"));

    //buf2 = (uint8_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_8BIT);
    //if(buf2 == nullptr) Logger.Error(F("....Allocation of lower scroll buffer (buf2) did not succeed"));
    //else Logger.Info(F("....Allocation of lower scroll buffer (buf2) successful"));
    Logger.Info_f(F("....Free heap: %d"), ESP.getFreeHeap());
    Logger.Info_f(F("....Largest free block: %d"), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    Logger.Info(F("Done."));
}

/**
 * @brief Print string
 * @param st - the string to print
 * @param x - the x coordinate
 * @param y - the y coordinate
 * @param xo - x origin of the print area
 * @param yo - y origin of the print area
 */
size_t Controller_Display::print(uint8_t *st, int16_t x, int16_t y, int16_t xo, int16_t yo) 
{
  return Controller_Display::print(st, x, y, xo, yo);
};

/**
 * @brief Print a string in the working area. Advances the cursor to keep track of position
 * @param c - Font color to use.
 * @param s - String to print
 */
void Controller_Display::w_area_print(String s, uint16_t c, bool newline)
{
    if(!w_area_initialized)
    {
      fill_rect(w_area_x1, w_area_y1, w_area_x2-w_area_x1, w_area_y2-w_area_y1, 0x0000);
      w_area_initialized = true;
    }
    set_text_color(c);
    set_text_back_color(0x0);
    set_text_size(1);
    //while(w_area_y1 + w_area_cursor_y > w_area_y2 - text_size*8)
    //{
    //    // scroll display area to make space
    //    window_scroll(w_area_x1, w_area_y1, w_area_x2-w_area_x1, w_area_y2-w_area_y1, 
    //        0, text_size*8, buf1, buf2, text_size*8);
    //    w_area_cursor_y = w_area_cursor_y - text_size*8;
    //}

    print_string(s, w_area_cursor_x, w_area_cursor_y, w_area_x1, w_area_y1);
    w_area_cursor_x = get_text_X_cursor();
    w_area_cursor_y = get_text_Y_cursor();
    if(newline)
    {
        print_string("\n", w_area_cursor_x, w_area_cursor_y, w_area_x1, w_area_y1);
        w_area_cursor_x = 0;
        w_area_cursor_y = get_text_Y_cursor();
    }

}

void Controller_Display::draw_arrow(int16_t x, int16_t y, Direction d, uint8_t size, int16_t fg, int16_t bg)
{
    const uint8_t _w = 8;
    const uint8_t _h = 5;
    if(d == Direction::LEFT || d == Direction::RIGHT)
    {
      fill_rect(x, y, _w*size, _h*size, bg);
      int _height = _h * size; 
      int _width = 2 * size; 
      int mid = ((_height+1)&~1)/2 - 1;
      int c = 0;
      if(d==Direction::LEFT)
      {
        fill_rect(x+size+2, y+((_h*size+1)&~1)/2-size+1, _w*(size-1), size, fg);
        for(int i=0; i < 2-size%2 ; i++) for(int k=0; k<_width; k++) draw_pixel(x+size+k, y+mid+i, fg);
        for(int i=0; i < mid ; i++)
        {
          c++;
          for(int k=c; k<_width; k++)
          {
              draw_pixel(x+size+k, y+mid-1-i, fg);
              draw_pixel(x+size+k, y+mid+1+i, fg);
          }
        } 
      }
      else
      {  
        fill_rect(x+size, y+((_h*size+1)&~1)/2-size+1, _w*(size-1), size, fg);
        for(int i=0; i < 2-size%2 ; i++) for(int k=0; k<_width; k++) draw_pixel(x-k+(_w*size)-size-1, y+mid+i, fg);
        for(int i=0; i < mid ; i++)
        {
          c++;
          for(int k=c; k<_width; k++)
          {
              draw_pixel(x+_w*size-k-4, y+mid-1-i, fg);
              draw_pixel(x+_w*size-k-4, y+mid+1+i, fg);
          }
        } 
      }
    }
    else
    {
      int _width = _h * size; 
      int _height = 2 * size; 
      int mid = ((_width+1)&~1)/2 - 1;
      int c = 0;
      fill_rect(x, y, _h*size, _w*size, bg);
      if(d==Direction::UP)
      {
        fill_rect(x+((_h*size+1)&~1)/2-size+1, y+size+2, size, _w*(size-1), fg);
        for(int i=0; i < 2-size%2 ; i++) for(int k=0; k<_width; k++) draw_pixel(x+mid+i, y+size+k, fg);
        for(int i=0; i < mid ; i++)
        {
          c++;
          for(int k=c; k<_height; k++)
          {
              draw_pixel(x+mid-1-i, y+size+k ,fg);
              draw_pixel(x+mid+1+i, y+size+k ,fg);
          }
        } 
      }   
      else
      {
        fill_rect(x+((_h*size+1)&~1)/2-size+1, y+size, size, _w*(size-1), fg);
        for(int i=0; i < 2-size%2 ; i++) for(int k=0; k<_width; k++) draw_pixel(x+mid+i, y+_w*size-k-size-1, fg);
        for(int i=0; i < mid ; i++)
        {
          c++;
          for(int k=c; k<_height; k++)
          {
              draw_pixel(x+mid-1-i, y+_w*size-k-size-1 ,fg);
              draw_pixel(x+mid+1+i, y+_w*size-k-size-1 ,fg);
          }
        } 
      }   
    }
}

/**
 * @brief Implements scrolling for partial screen, both horizontally and vertically
 * @param x - the top left of the scrolling area x coordinate
 * @param y - the top left of the scrolling area y coordinate
 * @param w - the width of the scroll area
 * @param h - the height of the scorll area
 * @param dx - the ammount to scroll into the x direction
 * @param dy - the ammount to scroll tino the y direction
 * @param bufh - the upper page buffer. Expected to be w*h*3/2
 * @param bufl - the lower page buffer. Expected to be w*h*3/2
 * @param inc - the scroll imcrement. Defaults to 1. 
 * @remarks Since memory on the ESP32 is limited, the page buffer is split into an upper and lower area
 * to accomodate an scrolling window of about 280*180 (or a bit larger). The buffers needs to be allocated to 
 * be each w*h/2*3 (since each pixel is represented by 3 bytes). Additionally, it is important that the dy 
 * parameter is divisible by two and that dy/2 is divisible by inc. 
 */
void Controller_Display::window_scroll(int16_t x, int16_t y, int16_t w, int16_t h, int16_t dx, int16_t dy, uint8_t *bufh, uint8_t *bufl, uint8_t inc)
{
    uint32_t cnth = 0;
    uint32_t cntl = 0;
    uint16_t bh = h/2;
    if (dx) 
    {
      // even though the ILI9488 does support vertical scrolling (horizontal in landscape) via hardware
      // it only scrolls the entire screen height. So we still need to implement software scrolling...
      // first we read the data....
      cnth = read_GRAM_RGB(x, y, bufh, w, bh);
      cntl = read_GRAM_RGB(x, y + bh, bufl, w, bh);
      
      // to scroll by 1 pixel to the right, we need to process each row in the read buffer
      // and move the pixel bytes over by one and then blank our the first pixel....
      // to affect the scrolling distance dx, we need to iterate until we reach dx...
      for(uint16_t i=1; i<=dx; i+=inc)
      {
        for(uint16_t row=0; row<bh; row++)
        {
          uint8_t *rowStarth = &bufh[row*w*3];
          uint8_t *rowStartl = &bufl[row*w*3];                            // position the pointer at the start of the row.
          memmove(rowStarth + (i+inc-1)*3, rowStarth + (i-1)*3, (w - (i+inc-1)) * 3); 
          memmove(rowStartl + (i+inc-1)*3, rowStartl + (i-1)*3, (w - (i+inc-1)) * 3); 
                                                                          // move the bytes over appropriately
          for(uint8_t k=1; k<=inc; k++)
          {                                                                 
            rowStarth[((i-1)+(k-1))*3]     = 0x0;               // Red component 
            rowStarth[((i-1)+(k-1))*3 + 1] = 0x0;               // Green component 
            rowStarth[((i-1)+(k-1))*3 + 2] = 0x0;               // Blue component 
            rowStartl[((i-1)+(k-1))*3]     = 0x0;               // Red component 
            rowStartl[((i-1)+(k-1))*3 + 1] = 0x0;               // Green component 
            rowStartl[((i-1)+(k-1))*3 + 2] = 0x0;               // Blue component 
                                                                // Set the first pixel to black (0x0, 0x0, 0x0)
          }
        }
        set_addr_window(x, y, x+w-1, y+h-1);                              // Set the scroll region data window
        CS_ACTIVE;
        writeCmd8(CC);
        CD_DATA;
        spi->transferBytes(bufh, nullptr, cnth);                            // transfer the updated buffer into the window.
        spi->transferBytes(bufl, nullptr, cntl);
        CS_IDLE;
      }
    }
    if (dy) 
    {
      // if we scroll vertically and rotation is landscape, we need to scroll in software
      // first we read the data....
      cnth = read_GRAM_RGB(x, y, bufh, w, bh);
      cntl = read_GRAM_RGB(x, y + bh, bufl, w, bh);
      set_addr_window(x, y, x + w-1, y+h-1);
      CS_ACTIVE;
      writeCmd8(CC);
      CD_DATA;

      for(uint16_t i=inc; i<=dy; i+=inc)
      {
        if(i<=bh)
        {
          spi->transferBytes(bufh + i*3*w, nullptr, cnth-3*i*w);
          spi->transferBytes(bufl, nullptr, cntl);
          memset(bufh + 3*(i-inc)*w, 0x0, inc*w*3);
          spi->transferBytes(bufh, nullptr, 3*i*w);
                // each dy means we have to move the start over by 3* the with of the area
                // conversely, the size to transfer reduces by dy*3*width
                // but now the last row needs to be blanked....
        }
        else
        {
          // now bufh has been fully processes and we need to shif processing to bufl
          spi->transferBytes(bufl + (i-bh)*3*w, nullptr, cntl-3*(i-bh)*w);
          spi->transferBytes(bufh, nullptr, cnth);
          memset(bufl + 3*(i-bh-inc)*w, 0x0, inc*w*3);
          spi->transferBytes(bufl, nullptr, 3*(i-bh)*w);
        }
      }
      CS_IDLE;
    }
}

/**
 * @brief Tests the display by going through a routine of drawing various
 * shapes and information
 */
void Controller_Display::test()
{
 
  int w = w_area_x2 - w_area_x1;
  int h = w_area_y2 - w_area_y1;

  draw_background(lcars, lcars_size);
  fill_rect(w_area_x1, w_area_y1, w, h, 0xf800); vTaskDelay(500);
  fill_rect(w_area_x1, w_area_y1, w, h, 0x07E0); vTaskDelay(500);
  fill_rect(w_area_x1, w_area_y1, w, h, 0x001F); vTaskDelay(500);
  fill_rect(w_area_x1, w_area_y1, w, h, 0x0);
  for(int i=0;i<50;i++)
  {
    set_draw_color(random(65535));
    draw_rectangle(
      w_area_x1 + random(w), 
      w_area_y1 + random(h),
      w_area_x1 + random(w),
      w_area_y1 + random(h)
    );
    vTaskDelay(100);
  }
  fill_rect(w_area_x1, w_area_y1, w, h, 0x0);
  for(int i=0;i<50;i++)
  {
    int x1 = w_area_x1 + random(w);
    int y1 = w_area_y1 + random(h);
    int x2 = w_area_x1 + random(w);
    int y2 = w_area_y1 + random(h);
    int r = (x2>x1?x2-x1:x1-x2)>(y2>y1?y2-y1:y1-y2) ? random((y2>y1?y2-y1:y1-y2)/4) : random((x2>x1?x2-x1:x1-x2)/4);
    set_draw_color(random(65535));
    draw_round_rectangle(x1, y1, x2, y2, r); 
    vTaskDelay(100);
  }
  fill_rect(w_area_x1, w_area_y1, w, h, 0x0);
  for(int i=0;i<50;i++)
  {
    set_draw_color(random(65535));
    draw_triangle(
      w_area_x1 + random(w), 
      w_area_y1 + random(h),
      w_area_x1 + random(w), 
      w_area_y1 + random(h), 
      w_area_x1 + random(w), 
      w_area_y1 + random(h)
    );
  }
  fill_rect(w_area_x1, w_area_y1, w, h, 0x0);
  for(int i=0;i<50;i++)
  {
    uint16_t r = w>h ? random(h/2) : random(w/2);
    set_draw_color(random(65535));
    draw_circle(
      w_area_x1 + r + random(w), 
      w_area_y1 + r + random(h),
      r
    );
  }
  fill_rect(w_area_x1, w_area_y1, w, h+4, 0x0);
  set_text_back_color(0x0);
  set_text_color(0xf800);
  set_text_size(5);
  print_string("The End", w_area_x1+20, w_area_y1+20);

  vTaskDelay(5000);
  fill_rect(w_area_x1, w_area_y1, w, h, 0x0);
}

/**
 * @brief Writes the currently selected Axis into the display
 * @param axis - The value for the axis to print
 */
void Controller_Display::write_axis(Axis axis)
{
    set_text_back_color(RGB_to_565(177,0,254));
    set_text_color(0xffff);
    set_text_size(4);
    switch(axis)
    {
        case X: print_string("X", 155, 158); break;
        case Y: print_string("Y", 155, 158); break;
        case Z: print_string("Z", 155, 158); break;
    }

}

/**
 * @brief Writes the last command into the display
 * @param c - the command name.
 */
void Controller_Display::write_command(String c)
{
    fill_rect(136, 101, 300, 9, RGB_to_565(127,106,0));
    set_text_back_color(RGB_to_565(127,106,0));
    set_text_color(0xffffff);
    set_text_size(1);
    print_string(c, 141, 101);  
}

/**
 * @brief Writes emergency indicator to the disaply
 * @param has_emergency - Whether there is an emergency or not.
*/
void Controller_Display::write_emergency(bool has_emergency)
{
  if(has_emergency)
  {
    fill_rect(0, 50, 66, 24, 0xf800);
    fill_rect(66, 51, 1, 23, RGB_to_565(0x7f,0x0,0x0));
  }
  else
  {
    fill_rect(0, 50, 66, 24, RGB_to_565(75, 255, 0));
    fill_rect(1,50, 64, 1, RGB_to_565(28, 87, 3));
    fill_rect(1,73, 64, 1, RGB_to_565(61, 207, 0));
    fill_rect(0,51, 1, 22, RGB_to_565(51, 201, 0));
    fill_rect(66,51, 1, 22, RGB_to_565(61, 140, 25));   
    draw_pixel(0, 50, 0x0);
    draw_pixel(66, 50, 0x0);
    draw_pixel(0, 73, RGB_to_565(26, 145, 0));
    draw_pixel(66, 73, RGB_to_565(39, 102, 11)); 
  }
}

/**
 * @brief Writes the current feed selection to the display
 * @param feed - The value for the feed selection to write
 */
void Controller_Display::write_feed(float feed)
{
    set_text_back_color(RGB_to_565(177,0,254));
    set_text_color(0xffffff);
    set_text_size(3);
    print_number_float(feed, 3, 90, 210, '.', 5, ' ');
}

/**
 * @brief Writes a status message to  the display
 * @param format - format sting for the message.
 * @param ... Argument list for the token replacement in the format string.
 */
void Controller_Display::write_status(const String &format, ...)
{
    char *buf = NULL;
    va_list copy;
    va_list args; 

    fill_rect(136, 114, 300, 9, RGB_to_565(127,106,0));
    set_text_back_color(RGB_to_565(127,106,0));
    set_text_color(0xffffff);
    set_text_size(1);

    // Determine the size of the formatted string 
    va_start(args, format); 
    va_copy(copy, args);
    int len = vsnprintf(nullptr, 0, format.c_str(), args);
    if(len < 0) {
      // error condition, most likely in the format string. 
      va_end(args);
      va_end(copy);
      return;
    };
    
    // allocate memory for the operation
    buf = (char*) malloc(len+1);
    if(buf == NULL) {
      // memory allocation error.
      va_end(args); 
      va_end(copy);
      return;
    }

    // Format the string  
    vsnprintf(buf, len+1, format.c_str(), copy);
    va_end(copy); 
    va_end(args);

    print_string(String(buf), 141, 114);
    free(buf);  
}

/**
 * @brief Writes the current x position to the display
 * @param x - The value for the x position to write
 */
void Controller_Display::write_x(float x)
{
    set_text_back_color(RGB_to_565(177,0,254));
    set_text_color(0xffffff);
    set_text_size(2);
    print_number_float(x, 3, 128, 65, '.', 7, ' ');
}

/**
 * @brief Writes the current y position to the display
 * @param y - The value for the y position to write
 */
void Controller_Display::write_y(float y)
{
    set_text_back_color(RGB_to_565(177,0,254));
    set_text_color(0xffffff);
    set_text_size(2);
    print_number_float(y, 3, 238, 65, '.', 7, ' ');
}

/**
 * @brief Writes the current z position to the display
 * @param z - The value for the z position to write
 */
void Controller_Display::write_z(float z)
{
    set_text_back_color(RGB_to_565(177,0,254));
    set_text_color(0xffffff);
    set_text_size(2);
    print_number_float(z, 3, 348, 65, '.', 7, ' ');
}