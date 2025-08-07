// Copyright (c) Thor Schueler. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include "Arduino.h"
#include "../controller_display/controller_display.h"



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
         * @brief Event handler monitoring the Axus GPIOs 
         */
        void IRAM_ATTR handle_axis_change(); 

        /**
         * @brief Event handler monitoring the Emergency Shutdown Button 
         */
        void IRAM_ATTR handle_ems_change(); 

        /**
         * @brief Event handler watching the Quadradure encoder GPIOs.
         */
        void IRAM_ATTR handle_encoder_change();

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

        SemaphoreHandle_t _display_mutex;
        
};

#endif