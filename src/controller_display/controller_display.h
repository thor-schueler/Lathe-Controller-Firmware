// Copyright (c) Thor Schueler. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _DISPLAY_WHEEL_H_
#define _DISPLAY_WHEEL_H_

#include "Arduino.h"
#include "../display_spi/display_spi.h"

#pragma region externals for icons
extern const unsigned char lcars[] PROGMEM;
extern const unsigned char ems[] PROGMEM;
extern const size_t lcars_size;
extern const size_t ems_size;

extern const unsigned char engine_on[] PROGMEM;
extern const unsigned char engine_off[] PROGMEM;
extern const unsigned int engine_x;
extern const unsigned int engine_y;
extern const unsigned int engine_w;
extern const unsigned int engine_h;
extern const size_t engine_on_size;
extern const size_t engine_off_size;

extern const unsigned char power_on[] PROGMEM;
extern const unsigned char power_off[] PROGMEM;
extern const unsigned int power_x;
extern const unsigned int power_y;
extern const unsigned int power_w;
extern const unsigned int power_h;
extern const size_t power_on_size;
extern const size_t power_off_size;

extern const unsigned char forward_on[] PROGMEM;
extern const unsigned char forward_off[] PROGMEM;
extern const unsigned int forward_x;
extern const unsigned int forward_y;
extern const unsigned int forward_w;
extern const unsigned int forward_h;
extern const size_t forward_on_size;
extern const size_t forward_off_size;

extern const unsigned char warning_off[] PROGMEM;
extern const unsigned char warning_pending_engine[] PROGMEM;
extern const unsigned int warning_x;
extern const unsigned int warning_y;
extern const unsigned int warning_w;
extern const unsigned int warning_h;
extern const size_t warning_off_size;
extern const size_t warning_pending_engine_size;

extern const unsigned char neutral_on[] PROGMEM;
extern const unsigned char neutral_off[] PROGMEM;
extern const unsigned int neutral_x;
extern const unsigned int neutral_y;
extern const unsigned int neutral_w;
extern const unsigned int neutral_h;
extern const size_t neutral_on_size;
extern const size_t neutral_off_size;

extern const unsigned char backward_on[] PROGMEM;
extern const unsigned char backward_off[] PROGMEM;
extern const unsigned int backward_x;
extern const unsigned int backward_y;
extern const unsigned int backward_w;
extern const unsigned int backward_h;
extern const size_t backward_on_size;
extern const size_t backward_off_size;

extern const unsigned char light_on[] PROGMEM;
extern const unsigned char light_off[] PROGMEM;
extern const unsigned int light_x;
extern const unsigned int light_y;
extern const unsigned int light_w;
extern const unsigned int light_h;
extern const size_t light_on_size;
extern const size_t light_off_size;

extern const unsigned char backlight_on[] PROGMEM;
extern const unsigned char backlight_off[] PROGMEM;
extern const unsigned int backlight_x;
extern const unsigned int backlight_y;
extern const unsigned int backlight_w;
extern const unsigned int backlight_h;
extern const size_t backlight_on_size;
extern const size_t backlight_off_size;

extern const unsigned char lube_on[] PROGMEM;
extern const unsigned char lube_off[] PROGMEM;
extern const unsigned int lube_x;
extern const unsigned int lube_y;
extern const unsigned int lube_w;
extern const unsigned int lube_h;
extern const size_t lube_on_size;
extern const size_t lube_off_size;
#pragma endregion

#pragma region externals for digits
extern const unsigned char* digits[10] PROGMEM;
extern const size_t digit_size[10] PROGMEM;
extern const unsigned int digit_width[10] PROGMEM;
extern const unsigned int digit_h;
#pragma endregion

#pragma region externals for scales
extern const unsigned int scales_h;
extern const unsigned int scales_y;
extern const unsigned char* scales_o[6] PROGMEM;
extern const unsigned char* scales_g[6] PROGMEM;
extern const unsigned int scales_width[6] PROGMEM;
extern const unsigned int scales_x[6] PROGMEM;
extern const size_t scales_size[6] PROGMEM;
extern const unsigned int speeds[6] PROGMEM;
#pragma endregion

/**
 * @brief Implements the display for the handwheel
 */
class Controller_Display:public DISPLAY_SPI
{
	public:
		/**
		 * @brief Generates a new instance of the DISPLAY_SPI class. 
		 * @details initializes the SPI and LCD pins including CS, RS, RESET 
		 */
		Controller_Display();

		/**
		 * @brief Initializes the display
		 */
		void init();

		/**
		 * @brief Tests the display by going through a routine of drawing various
		 * shapes and information
		 */
		void test();

		/**
		 * @brief Draws the standard background.
		 */
		void update_background();

		/**
		 * @brief Updates the back light state icon.
		 * @param lighted - True if light it on, false otherwise.
		*/
		void update_back_light(bool lighted);

		/**
		 * @brief Updates the engine state icon.
		 * @param energized - True if state it engergized, false otherwise.
		*/
		void update_engine_state(bool energized);

		/**
		 * @brief Updates the FOR status icons.
		 * @param for_f - True if FOR forward switch is on.
		 * @param for_b - True if FOR backward switch is on.
		*/
		void update_for_state(bool for_f, bool for_b);

		/**
		 * @brief Updates the light state icon.
		 * @param lighted - True if light it on, false otherwise.
		*/
		void update_light_state(bool lighted);

		/**
		 * @brief Updates the lube state icon.
		 * @param active - True if lube is on, false otherwise.
		*/
		void update_lube_state(bool active);

		/**
		 * @brief Updates the power state icon.
		 * @param energized - True if state it powered, false otherwise.
		*/
		void update_power_state(bool powered);

		/**
		 * @brief Updates the warning area.
		 * @param has_deferred_action - True if a deferred action is pending, false otherwise.
		 */
		void update_warning(bool has_deferred_action);

		/**
		 * @brief Writes emergency indicator to the disaply
		*/
		void write_emergency();

		/**
		 * @brief Writes the current rpm to the display
		 * @param rpm - The value for the rpm to write
		 */
		void write_rpm(unsigned int rpm);

		/**
		 * @brief Updates the scale display according to the given speed
		 * @param rpm - The speed in rotations per minute
		 */
		void update_scale(unsigned int rpm);

	private: 

		uint8_t* buf1 = nullptr;
		uint8_t* buf2 = nullptr;
		bool w_area_initialized = false;
};

#endif