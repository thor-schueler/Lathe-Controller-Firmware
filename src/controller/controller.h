// Copyright (c) Thor Schueler. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <Arduino.h>
#include "../controller_display/controller_display.h"

#define DEBOUNCE_MS 150

#define I_MAIN_POWER 4
#define I_EMS 34
#define I_ENERGIZE 39
#define I_FOR_F 35
#define I_FOR_B 36
#define I_LIGHT 22
#define I_CONTROLBOARD_DETECT 27
#define I_SPINDLE_PULSE 33

#define O_SPINDLE_DIRECTION_SWITCH_A 21
#define O_SPINDLE_DIRECTION_SWITCH_B 19
#define O_SPINDLE_OFF 18
#define O_ENGINE_DISCHARGE 39

#define HALL_DEBOUNCE_DELAY_US 50
#define HALL_POLLING_INTERVAL_US 50
#define USE_POLLING_FOR_RPM false
    // this define controls whether we use polling on a timer or interrupts for RPM measurement. 
    // while interrupt drive is preferable, there seem to be a lot of phantom interrupts on low RPMs
    // probably because the slow takeup edge on the hall. We might be able to reduce the capacitor 
    // from the Hall input to ground to steepen the edge, but it is more likely a function of the slow
    // increase of the magnetic field on low RPMs (on higher RPMs, the edge is sharp).
    // see also https://github.com/espressif/arduino-esp32/issues/4172 and 
    // https://github.com/espressif/esp-idf/issues/7602 for addtional discussion. 



/**
 * @brief Defines the state of an input or output
 */
struct State {
    bool desired = false;
    bool reported = false;
};

/**
 * @brief Implements the basic controller functionality
 */
class Controller
{
    public:
        /**
         * @brief Creates a new instance of Controller
         */
        Controller();
    
    protected:

        /**
         * @brief Task function managing the display
         * @param args - pointer to task arguments
         */
        static void display_runner(void* args);

        /**
         * @brief Task function managing the various user input states.
         * @param args - pointer to task arguments 
         */
        static void input_runner(void* args); 

        /**
         * @brief Interrupt handler to read the State of the Hall sensor to measure Motor RPM
         *
         * @param arg pointer to class instance context (this)
         */
        static void IRAM_ATTR read_hall_sensor(void *arg);

        /**
         * @brief Event handler watching for changes on the energize button toggle.
         */
        void IRAM_ATTR handle_energize();

        /**
         * @brief Event handler watching for changes on any inputs.
         */
        void IRAM_ATTR handle_input();

        /**
         * @brief Event handler monitoring the Spindle Pulse
         */
        void IRAM_ATTR handle_spindle_pulse(); 

    private: 

        /**
         * @brief Formats a string, essentially a wrapper for vnsprintf
         * @param format - format string
         * @param ... - variable argument list 
         */
        String format_string(const char* format, ...);
    
        Controller_Display *_display = nullptr;
    
        TaskHandle_t _displayRunner;
        TaskHandle_t _inputRunner;

        volatile bool _main_power = false;
        volatile bool _has_emergency = false;
        volatile bool _toggle_energize = false;
        volatile bool _for_f = false;
        volatile bool _for_b = false;
        volatile bool _light = false;
        volatile bool _is_energized = false;

        volatile State _direction_a;
        volatile State _direction_b;
        volatile State _common;
        volatile State _deenergize;

        unsigned long _last_toggle_energize = 0;
        unsigned long _last_input_change = 0;
        unsigned long _hall_debounce_tick = 0;
        unsigned int _revolutions = 0;


        SemaphoreHandle_t _display_mutex;
        esp_timer_handle_t _hall_timer_handle = NULL;        
};

#endif