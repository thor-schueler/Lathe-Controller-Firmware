// Copyright (c) Thor Schueler. All rights reserved.
// SPDX-License-Identifier: MIT
// IMPORTANT: LIBRARY MUST BE SPECIFICALLY CONFIGURED FOR EITHER TFT SHIELD
// OR BREAKOUT BOARD USAGE.

#include <SPI.h>
#include "controller_display.h"
#include "../logging/SerialLogger.h"

const unsigned int rpm_x = 218;
const unsigned int rpm_y = 43;

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
    fill_rect(0, 0, this->width, this->height, 0x0);
    
    Logger.Info_f(F("....Free heap: %d"), ESP.getFreeHeap());
    Logger.Info_f(F("....Largest free block: %d"), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    Logger.Info(F("....Done."));
}

/**
 * @brief Tests the display by going through a routine of drawing various
 * shapes and information
 */
void Controller_Display::test()
{ 
  
  Logger.Info(F("Testing display..."));
  fill_rect(0, 0, this->width, this->height, 0xf800); vTaskDelay(500);
  fill_rect(0, 0, this->width, this->height, 0x07E0); vTaskDelay(500);
  fill_rect(0, 0, this->width, this->height, 0x001F); vTaskDelay(500);
  fill_rect(0, 0, this->width, this->height, 0x0); vTaskDelay(500);
  draw_background(lcars, lcars_size);
  Logger.Info(F("Testing display... done."));
  return;
}

/**
 * @brief Draws the standard background.
 */
void Controller_Display::update_background()
{
    draw_background(lcars, lcars_size);
    fill_rect(70, rpm_y, rpm_x, digit_h+5, 0x0);
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
  draw_image((!for_f && !for_b) ? neutral_on : neutral_off, (!for_f && !for_b) ? neutral_on_size : neutral_off_size, neutral_x, neutral_y, neutral_w, neutral_y);
  draw_image((!for_f && for_b) ? backward_on : backward_off, (!for_f && for_b) ? backward_on_size : backward_off_size, backward_x, backward_y, backward_w, backward_y);
}

/**
 * @brief Updates the light state icon.
 * @param lighted - True if light it on, false otherwise.
 */
void Controller_Display::update_light_state(bool lighted)
{
  draw_image(lighted ? light_off : light_on, lighted ? light_off_size : light_on_size, light_x, light_y, light_w, light_h);
}

/**
 * @brief Updates the lube state icon.
 * @param active - True if lube is on, false otherwise.
 */
void Controller_Display::update_lube_state(bool active)
{
  draw_image(active ? lube_off : lube_on, active ? lube_off_size : lube_on_size, lube_x, lube_y, lube_w, lube_h);
}

/**
 * @brief Updates the power state icon.
 * @param energized - True if state it powered, false otherwise.
 */
void Controller_Display::update_power_state(bool powered)
{
  draw_image(powered ? power_off : power_on, powered ? power_off_size : power_on_size, power_x, power_y, power_w, power_h);
}

/**
 * @brief Updates the warning area.
 * @param has_deferred_action - True if a deferred action is pending, false otherwise.
 */
void Controller_Display::update_warning(bool has_deferred_action)
{
  if(has_deferred_action)
  {
    draw_image(warning_pending_engine, warning_pending_engine_size, warning_x, warning_y, warning_w, warning_h);
  }
  else
  {
    draw_image(warning_off, warning_off_size, warning_x, warning_y, warning_w, warning_h);
  }
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
    
  // Clear area on first invoation.
  static bool is_first = true;
  if(is_first)
  {
    uint16_t n = (rpm_x - 70) * digit_h * 2;
    uint8_t* blank = (uint8_t*)malloc(n);
    if (blank) {
        memset(blank, 0x00, n);
        draw_image(blank, n, 70, rpm_y, rpm_x-70, digit_h);
        free(blank);
    }
    is_first = false;
  }

  // Extract digits into a buffer (max 4 digits)
  static int8_t current_digits[4] = { -1, -1, -1, -1 }; 
  int8_t digit[4] = { -1, -1, -1, -1};
  uint8_t count = 0;
  do 
  {
      digit[count++] = rpm % 10;
      rpm /= 10;
  } while (rpm > 0 && count < 4);

  // Render only digits that have changed...
  int16_t x = rpm_x;
  for(int i=0; i<4; i++)
  {
    if(digit[i] == current_digits[i])
    {
        // digit has not changed, so nothing....
    }
    else
    {
      if(digit[i] == -1)
      {
        // digit is now blank, so clear it
        uint8_t w = digit_width[0];
        x -= w;
        fill_rect(x, rpm_y, w, digit_h, 0x0);

      }
      else
      {
        // digit has changed, so redraw it
        uint8_t dig = digit[i];
        uint8_t w = digit_width[dig];
        x -= w;
        draw_image(digits[dig], digit_size[dig], x, rpm_y, w, digit_h);
      }
      current_digits[i] = digit[i];
    }
  }  
}

/**
 * @brief Updates the scale display according to the given speed
 * @param rpm - The speed in rotations per minute
 */
void Controller_Display::update_scale(unsigned int rpm)
{
    static unsigned int current_rpm = -1;
    static unsigned int current_scale[6] = {0,0,0,0,0,0};
    if(current_rpm != rpm)
    {
      // need to update sales... 
      current_rpm = rpm;
      if(current_rpm <= 0) 
      {
        for(int i=0; i<6; i++) if(current_scale[i] != 0) 
        {
          current_scale[i] = 0;
          draw_image(scales_o[i], scales_size[i], scales_x[i], scales_y, scales_width[i], scales_h);
        }
      }
      else if (current_rpm < speeds[0])
      {
        float p = (float)current_rpm / (float)speeds[0];
        int xp =  static_cast<int>(ceil(p * scales_width[0]));
        current_scale[0] = p;

        // generate a partial scale images based on the percentage
        unsigned char* output_g = static_cast<unsigned char*>(malloc(scales_h*xp*2));
        unsigned char* output_y = static_cast<unsigned char*>(malloc(scales_h*(scales_width[0] - xp)*2));
        if(output_g && output_y)
        {
          for(int row=0; row<scales_h; row++)
          {
            memcpy(&output_g[row*xp*2], &scales_g[0][row*scales_width[0]*2], xp*2);
            memcpy(&output_y[row*(scales_width[0]-xp)*2], &scales_o[0][row*scales_width[0]*2 + xp*2], (scales_width[0]-xp)*2);
          }
          draw_image(output_g, scales_h*xp*2, scales_x[0], scales_y, xp, scales_h);
          draw_image(output_y, scales_h*(scales_width[0]-xp)*2, scales_x[0]+xp, scales_y, scales_width[0]-xp, scales_h);
          free(output_g);
          free(output_y);
        }
        else
        {
          // memory allocation failed... just draw the full scale
          draw_image(scales_o[0], scales_size[0], scales_x[0], scales_y, scales_width[0], scales_h);
        }
        // draw the other bars as necessary
        for(int i=1; i<6; i++) if(current_scale[i] != 0) 
        {
          current_scale[i] = 0;
          draw_image(scales_o[i], scales_size[i], scales_x[i], scales_y, scales_width[i], scales_h);
        }
      }
      else if (current_rpm < speeds[1])
      {
        // draw full green bars as necessary
        for(int i=0; i<1; i++) if(current_scale[i] != 0) 
        {
          current_scale[i] = 0;
          draw_image(scales_o[i], scales_size[i], scales_x[i], scales_y, scales_width[i], scales_h);
        }

        float p = (float)(current_rpm - speeds[0]) / (float)(speeds[1]-speeds[0]);
        int xp =  static_cast<int>(ceil(p * scales_width[1]));
        current_scale[1] = p;

        // generate a partial scale images based on the percentage
        unsigned char* output_g = static_cast<unsigned char*>(malloc(scales_h*xp*2));
        unsigned char* output_y = static_cast<unsigned char*>(malloc(scales_h*(scales_width[1] - xp)*2));
        if(output_g && output_y)
        {
          for(int row=0; row<scales_h; row++)
          {
            memcpy(&output_g[row*xp*2], &scales_g[1][row*scales_width[1]*2], xp*2);
            memcpy(&output_y[row*(scales_width[1]-xp)*2], &scales_o[1][row*scales_width[1]*2 + xp*2], (scales_width[1]-xp)*2);
          }
          draw_image(output_g, scales_h*xp*2, scales_x[1], scales_y, xp, scales_h);
          draw_image(output_y, scales_h*(scales_width[1]-xp)*2, scales_x[1]+xp, scales_y, scales_width[1]-xp, scales_h);
          free(output_g);
          free(output_y);
        }
        else
        {
          // memory allocation failed... just draw the full scale
          draw_image(scales_o[1], scales_size[1], scales_x[1], scales_y, scales_width[1], scales_h);
        }

        // draw the other bars as necessary
        for(int i=2; i<6; i++) if(current_scale[i] != 0) 
        {
          current_scale[i] = 0;
          draw_image(scales_o[i], scales_size[i], scales_x[i], scales_y, scales_width[i], scales_h);
        }
      }
      else if (current_rpm < speeds[2])
      {
        // draw full green bars as necessary
        for(int i=0; i<2; i++) if(current_scale[i] != 0) 
        {
          current_scale[i] = 0;
          draw_image(scales_o[i], scales_size[i], scales_x[i], scales_y, scales_width[i], scales_h);
        }

        float p = (float)(current_rpm - speeds[1]) / (float)(speeds[2]-speeds[1]);
        int xp =  static_cast<int>(ceil(p * scales_width[2]));
        current_scale[2] = p;

        // generate a partial scale images based on the percentage
        unsigned char* output_g = static_cast<unsigned char*>(malloc(scales_h*xp*2));
        unsigned char* output_y = static_cast<unsigned char*>(malloc(scales_h*(scales_width[2] - xp)*2));
        if(output_g && output_y)
        {
          for(int row=0; row<scales_h; row++)
          {
            memcpy(&output_g[row*xp*2], &scales_g[2][row*scales_width[2]*2], xp*2);
            memcpy(&output_y[row*(scales_width[2]-xp)*2], &scales_o[2][row*scales_width[2]*2 + xp*2], (scales_width[2]-xp)*2);
          }
          draw_image(output_g, scales_h*xp*2, scales_x[2], scales_y, xp, scales_h);
          draw_image(output_y, scales_h*(scales_width[2]-xp)*2, scales_x[2]+xp, scales_y, scales_width[2]-xp, scales_h);
          free(output_g);
          free(output_y);
        }
        else
        {
          // memory allocation failed... just draw the full scale
          draw_image(scales_o[2], scales_size[2], scales_x[2], scales_y, scales_width[2], scales_h);
        }

        // draw the other bars as necessary
        for(int i=3; i<6; i++) if(current_scale[i] != 0) 
        {
          current_scale[i] = 0;
          draw_image(scales_o[i], scales_size[i], scales_x[i], scales_y, scales_width[i], scales_h);
        }
      }
      else if (current_rpm < speeds[3])
      {
        // draw full green bars as necessary
        for(int i=0; i<3; i++) if(current_scale[i] != 0) 
        {
          current_scale[i] = 0;
          draw_image(scales_o[i], scales_size[i], scales_x[i], scales_y, scales_width[i], scales_h);
        }

        float p = (float)(current_rpm - speeds[2]) / (float)(speeds[3]-speeds[2]);
        int xp =  static_cast<int>(ceil(p * scales_width[3]));
        current_scale[3] = p;

        // generate a partial scale images based on the percentage
        unsigned char* output_g = static_cast<unsigned char*>(malloc(scales_h*xp*2));
        unsigned char* output_y = static_cast<unsigned char*>(malloc(scales_h*(scales_width[3] - xp)*2));
        if(output_g && output_y)
        {
          for(int row=0; row<scales_h; row++)
          {
            memcpy(&output_g[row*xp*2], &scales_g[3][row*scales_width[3]*2], xp*2);
            memcpy(&output_y[row*(scales_width[3]-xp)*2], &scales_o[3][row*scales_width[3]*2 + xp*2], (scales_width[3]-xp)*2);
          }
          draw_image(output_g, scales_h*xp*2, scales_x[3], scales_y, xp, scales_h);
          draw_image(output_y, scales_h*(scales_width[3]-xp)*2, scales_x[3]+xp, scales_y, scales_width[3]-xp, scales_h);
          free(output_g);
          free(output_y);
        }
        else
        {
          // memory allocation failed... just draw the full scale
          draw_image(scales_o[3], scales_size[3], scales_x[3], scales_y, scales_width[3], scales_h);
        }

        // draw the other bars as necessary
        for(int i=4; i<6; i++) if(current_scale[i] != 0) 
        {
          current_scale[i] = 0;
          draw_image(scales_o[i], scales_size[i], scales_x[i], scales_y, scales_width[i], scales_h);
        }        
      }
      else if (current_rpm < speeds[4])
      {
        // draw full green bars as necessary
        for(int i=0; i<4; i++) if(current_scale[i] != 0) 
        {
          current_scale[i] = 0;
          draw_image(scales_o[i], scales_size[i], scales_x[i], scales_y, scales_width[i], scales_h);
        }

        float p = (float)(current_rpm - speeds[3]) / (float)(speeds[4]-speeds[3]);
        int xp =  static_cast<int>(ceil(p * scales_width[4]));
        current_scale[4] = p;

        // generate a partial scale images based on the percentage
        unsigned char* output_g = static_cast<unsigned char*>(malloc(scales_h*xp*2));
        unsigned char* output_y = static_cast<unsigned char*>(malloc(scales_h*(scales_width[4] - xp)*2));
        if(output_g && output_y)
        {
          for(int row=0; row<scales_h; row++)
          {
            memcpy(&output_g[row*xp*2], &scales_g[4][row*scales_width[4]*2], xp*2);
            memcpy(&output_y[row*(scales_width[4]-xp)*2], &scales_o[4][row*scales_width[4]*2 + xp*2], (scales_width[4]-xp)*2);
          }
          draw_image(output_g, scales_h*xp*2, scales_x[4], scales_y, xp, scales_h);
          draw_image(output_y, scales_h*(scales_width[4]-xp)*2, scales_x[4]+xp, scales_y, scales_width[4]-xp, scales_h);
          free(output_g);
          free(output_y);
        }
        else
        {
          // memory allocation failed... just draw the full scale
          draw_image(scales_o[4], scales_size[4], scales_x[4], scales_y, scales_width[4], scales_h);
        }

        // draw the other bars as necessary
        for(int i=5; i<6; i++) if(current_scale[i] != 0) 
        {
          current_scale[i] = 0;
          draw_image(scales_o[i], scales_size[i], scales_x[i], scales_y, scales_width[i], scales_h);
        } 
      }
      else if (current_rpm < speeds[5])
      {
        // draw full green bars as necessary
        for(int i=0; i<5; i++) if(current_scale[i] != 0) 
        {
          current_scale[i] = 0;
          draw_image(scales_o[i], scales_size[i], scales_x[i], scales_y, scales_width[i], scales_h);
        }

        float p = (float)(current_rpm - speeds[4]) / (float)(speeds[5]-speeds[4]);
        int xp =  static_cast<int>(ceil(p * scales_width[5]));
        current_scale[5] = p;

        // generate a partial scale images based on the percentage
        unsigned char* output_g = static_cast<unsigned char*>(malloc(scales_h*xp*2));
        unsigned char* output_y = static_cast<unsigned char*>(malloc(scales_h*(scales_width[5] - xp)*2));
        if(output_g && output_y)
        {
          for(int row=0; row<scales_h; row++)
          {
            memcpy(&output_g[row*xp*2], &scales_g[5][row*scales_width[5]*2], xp*2);
            memcpy(&output_y[row*(scales_width[5]-xp)*2], &scales_o[5][row*scales_width[5]*2 + xp*2], (scales_width[5]-xp)*2);
          }
          draw_image(output_g, scales_h*xp*2, scales_x[5], scales_y, xp, scales_h);
          draw_image(output_y, scales_h*(scales_width[5]-xp)*2, scales_x[5]+xp, scales_y, scales_width[5]-xp, scales_h);
          free(output_g);
          free(output_y);
        }
        else
        {
          // memory allocation failed... just draw the full scale
          draw_image(scales_o[5], scales_size[5], scales_x[5], scales_y, scales_width[5], scales_h);
        }
      }
      else if (current_rpm > speeds[5])
      {
        for(int i=0; i<6; i++) if(current_scale[i] != 100) 
        {
          current_scale[i] = 100;
          draw_image(scales_g[i], scales_size[i], scales_x[i], scales_y, scales_width[i], scales_h);
        }
      }
      else
      {
        // should never get here. take no action
      }
    }
}
