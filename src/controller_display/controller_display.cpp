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
    
    //toggle_backlight(false);
    Logger.Info_f(F("....Free heap: %d"), ESP.getFreeHeap());
    Logger.Info_f(F("....Largest free block: %d"), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    Logger.Info(F("....Done."));
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
 * @brief Tests the display by going through a routine of drawing various
 * shapes and information
 */
void Controller_Display::test()
{ 
  //draw_background(lcars, lcars_size);
  Logger.Info(F("Testing display..."));
  fill_rect(0, 0, this->width, this->height, 0xf800); vTaskDelay(500);
  fill_rect(0, 0, this->width, this->height, 0x07E0); vTaskDelay(500);
  fill_rect(0, 0, this->width, this->height, 0x001F); vTaskDelay(500);
  fill_rect(0, 0, this->width, this->height, 0x0);
  Logger.Info(F("Testing display... done."));
  return;

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

