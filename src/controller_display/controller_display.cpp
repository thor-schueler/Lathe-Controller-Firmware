// Copyright (c) Thor Schueler. All rights reserved.
// SPDX-License-Identifier: MIT
// IMPORTANT: LIBRARY MUST BE SPECIFICALLY CONFIGURED FOR EITHER TFT SHIELD
// OR BREAKOUT BOARD USAGE.

#include <SPI.h>
#include "controller_display.h"
#include "../logging/SerialLogger.h"

const unsigned int rpm_x = 218;
const unsigned int rpm_y = 50;

/**
 * @brief Generates a new instance of the Controller_Display class. 
 * @details initializes the SPI and LCD pins including CS, RS, RESET 
 */
Controller_Display::Controller_Display() {}

/**
 * @brief Initializes the display
 */
void Controller_Display::init()
{
    DISPLAY_SPI::init();
    //draw_background(lcars, lcars_size);
    test();
    
    toggle_backlight(false);
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
  return DISPLAY_GUI::print(st, x, y, xo, yo);
};

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
  draw_background(lcars, lcars_size);
  fill_rect(0, 0, this->width, this->height, 0xf800); vTaskDelay(500);
  fill_rect(0, 0, this->width, this->height, 0x07E0); vTaskDelay(500);
  fill_rect(0, 0, this->width, this->height, 0x001F); vTaskDelay(500);
  fill_rect(0, 0, this->width, this->height, 0x0);
  for(int i=0; i<50; i++)
  {
    set_draw_color(random(65535));
    draw_rectangle(
      0 + random(this->width), 
      0 + random(this->height),
      0 + random(this->width),
      0 + random(this->height)
    );
    vTaskDelay(100);
  }
  fill_rect(0, 0, this->width, this->height, 0x0);
  for(int i=0; i<50; i++)
  {
    int x1 = 0 + random(this->width);
    int y1 = 0 + random(this->height);
    int x2 = 0 + random(this->width);
    int y2 = 0 + random(this->height);
    int r = (x2>x1?x2-x1:x1-x2)>(y2>y1?y2-y1:y1-y2) ? random((y2>y1?y2-y1:y1-y2)/4) : random((x2>x1?x2-x1:x1-x2)/4);
    set_draw_color(random(65535));
    draw_round_rectangle(x1, y1, x2, y2, r); 
    vTaskDelay(100);
  }
  fill_rect(0, 0, this->width, this->height, 0x0);
  for(int i=0; i<50; i++)
  {
    set_draw_color(random(65535));
    draw_triangle(
      0 + random(this->width), 
      0 + random(this->height),
      0 + random(this->width), 
      0 + random(this->height), 
      0 + random(this->width), 
      0 + random(this->height)
    );
  }
  fill_rect(0, 0, this->width, this->height, 0x0);
  for(int i=0; i<50; i++)
  {
    uint16_t r = this->width > this->height ? random(this->height/2) : random(this->width/2);
    set_draw_color(random(65535));
    draw_circle(
      0 + r + random(this->width), 
      0 + r + random(this->height),
      r
    );
  }
  fill_rect(0, 0, this->width, this->height+4, 0x0);
  set_text_back_color(0x0);
  set_text_color(0xf800);
  set_text_size(3);
  print_string("The End", 20, 20);

  vTaskDelay(5000);
  fill_rect(0, 0, this->width, this->height, 0x0);
}

/**
 * @brief Updates the back light state icon.
 * @param lighted - True if light it on, false otherwise.
 */
void Controller_Display::update_back_light(bool lighted)
{
  draw_image(lighted ? backlight_on : backlight_off, lighted ? backlight_on_size : backlight_off_size, backlight_x, backlight_y, backlight_w, backlight_h);
}

/**
 * @brief Updates the engine state icon.
 * @param energized - True if state it engergized, false otherwise.
 */
void Controller_Display::update_engine_state(bool energized)
{
  const unsigned char* icon = energized ? engine_on : engine_off;
  const size_t size = energized ? engine_on_size : engine_off_size;
  draw_image(icon, size, engine_x, engine_y, engine_w, engine_h);
}

/**
 * @brief Updates the FOR status icons.
 * @param for_f - True if FOR forward switch is on.
 * @param for_b - True if FOR backward switch is on.
 */
void Controller_Display::update_for_state(bool for_f, bool for_b)
{
  draw_image((for_f && !for_b) ? forward_on : forward_off, (for_f && !for_b) ? forward_on_size : forward_off_size, forward_x, forward_y, forward_w, forward_y);
  draw_image((for_f && for_b) ? neutral_on : neutral_off, (for_f && for_b) ? neutral_on_size : neutral_off_size, neutral_x, neutral_y, neutral_w, neutral_y);
  draw_image((!for_f && for_b) ? backward_on : backward_off, (!for_f && for_b) ? backward_on_size : backward_off_size, backward_x, backward_y, backward_w, backward_y);
}

/**
 * @brief Updates the light state icon.
 * @param lighted - True if light it on, false otherwise.
 */
void Controller_Display::update_light_state(bool lighted)
{
  draw_image(lighted ? light_on : light_off, lighted ? light_on_size : light_off_size, light_x, light_y, light_w, light_h);
}

/**
 * @brief Updates the lube state icon.
 * @param active - True if lube is on, false otherwise.
 */
void Controller_Display::update_lube_state(bool active)
{
  draw_image(active ? lube_on : lube_off, active ? lube_on_size : lube_off_size, lube_x, lube_y, lube_w, lube_h);
}

/**
 * @brief Updates the power state icon.
 * @param energized - True if state it powered, false otherwise.
 */
void Controller_Display::update_power_state(bool powered)
{
  const unsigned char* icon = powered ? power_on : power_off;
  const size_t size = powered ? power_on_size : power_off_size;
  draw_image(icon, size, power_x, power_y, power_w, power_h);
}

/**
 * @brief Writes emergency indicator to the display
*/
void Controller_Display::write_emergency()
{
  this->draw_background(ems, ems_size);
}

/**
 * @brief Writes the current rpm to the display
 * @param rpm - The value for the rpm to write
 */
void Controller_Display::write_rpm(unsigned int rpm)
{
  if (rpm > 9999) rpm = 9999; // clamp to max
    
  // Extract digits into a buffer (max 4 digits)
  uint8_t digit[4];
  uint8_t count = 0;
  do {
      digit[count++] = rpm % 10;
      rpm /= 10;
  } while (rpm > 0 && count < 4);

  // Render digits from least to most significant (right to left)
  int16_t x = rpm_x;
  for (int i = 0; i < count; i++) {
    uint8_t dig = digit[i];
    uint8_t w = digit_width[dig];
    x -= w;
    draw_image(digits[dig], digit_size[dig], x, rpm_y, w, digit_h);
  }
}

