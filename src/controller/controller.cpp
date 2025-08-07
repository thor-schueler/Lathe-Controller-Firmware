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

#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <FunctionalInterrupt.h>
#include "controller.h"
#include "../logging/SerialLogger.h"

bool Wheel::_key_changed = false;
static Controller *_instance = nullptr;


/**
 * @brief Creates a new instance of Controller
 */
Controller::Controller()
{
    Logger.Info(F("Startup"));
    Logger.Info(F("....Initialize Display"));
    _display = new DISPLAY_Wheel();
    _display->set_rotation(3);
    _display->init();
    _instance = this;

    Logger.Info(F("....Inititialize GPIO pins"));
    pinMode(AXIS_Z, INPUT_PULLUP);
    pinMode(AXIS_X, INPUT_PULLUP);
    pinMode(AXIS_Y, INPUT_PULLUP);
    pinMode(EMS, INPUT_PULLUP);
    pinMode(WHEEL_A, INPUT_PULLUP);
    pinMode(WHEEL_B, INPUT_PULLUP);
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);

    Logger.Info(F("....Attach event receivers for GPIO"));
    attachInterrupt(digitalPinToInterrupt(AXIS_X), std::bind(&Wheel::handle_axis_change, this), FALLING);
    attachInterrupt(digitalPinToInterrupt(AXIS_Y), std::bind(&Wheel::handle_axis_change, this), FALLING);
    attachInterrupt(digitalPinToInterrupt(AXIS_Z), std::bind(&Wheel::handle_axis_change, this), FALLING);
    attachInterrupt(digitalPinToInterrupt(EMS), std::bind(&Wheel::handle_ems_change, this), CHANGE);
    attachInterrupt(digitalPinToInterrupt(WHEEL_A), std::bind(&Wheel::handle_encoder_change, this), CHANGE); 
    attachInterrupt(digitalPinToInterrupt(WHEEL_B), std::bind(&Wheel::handle_encoder_change, this), CHANGE);

    Logger.Info("....Initializing Axis and Feed Values");  
    if(!digitalRead(AXIS_X)) _selected_axis = Axis::X;
    else if(!digitalRead(AXIS_Y)) _selected_axis = Axis::Y;
    else if(!digitalRead(AXIS_Z)) _selected_axis = Axis::Z;  
    _has_emergency = digitalRead(EMS);

    Logger.Info(F("....Generating Mutexes"));
    _display_mutex = xSemaphoreCreateBinary();  xSemaphoreGive(_display_mutex);

    Logger.Info("....Create various tasks");
    xTaskCreatePinnedToCore(display_runner, "displayRunner", 8192, this, 1, &_displayRunner, 0);
    xTaskCreatePinnedToCore(input_runner, "inputRunner", 2048, this, 1, &_inputRunner, 0);

    Logger.Info("Startup done");
}


/**
 * @brief Task function managing the display
 * @param args - pointer to task arguments
 */
void Controller::display_runner(void* args)
{
    Wheel *_this = reinterpret_cast<Wheel *>(args);
    for (;;) 
    { 
        if (xSemaphoreTake(_this->_display_mutex, portMAX_DELAY) == pdTRUE) 
        {
            int16_t cf = _this->_display->RGB_to_565(0x00, 0xff, 0x00);
            int16_t cb = _this->_display->RGB_to_565(0xff, 0x00, 0x00);
            int16_t cn = _this->_display->RGB_to_565(177,0,254);
            int16_t cback = _this->_display->RGB_to_565(177,0,254);
            
            _this->_display->write_axis(_this->_selected_axis);
            _this->_display->write_feed(_this->_selected_feed);
            _this->_display->write_emergency(_this->_has_emergency);
            _this->_display->write_x(_this->_x);
            _this->_display->write_y(_this->_y);
            _this->_display->write_z(_this->_z);

            if(_this->_direction == 1)
            {
                _this->_display->draw_arrow(185, 14, Direction::RIGHT, 3, _this->_selected_axis == Axis::X ? cf : cn, cback);
                _this->_display->draw_arrow(305, 14, Direction::DOWN, 3, _this->_selected_axis == Axis::Y ? cb : cn, cback);
                _this->_display->draw_arrow(415, 14, Direction::UP, 3, _this->_selected_axis == Axis::Z ? cf : cn, cback);
            }
            else if(_this->_direction == -1)
            {
                _this->_display->draw_arrow(185, 14, Direction::LEFT, 3, _this->_selected_axis == Axis::X ? cb : cn, cback);
                _this->_display->draw_arrow(305, 14, Direction::UP, 3, _this->_selected_axis == Axis::Y ? cf : cn, cback);
                _this->_display->draw_arrow(415, 14, Direction::DOWN, 3, _this->_selected_axis == Axis::Z ? cb : cn, cback);
            }
            else
            {
                _this->_display->draw_arrow(185, 14, Direction::LEFT, 3, cn, cback);
                _this->_display->draw_arrow(305, 14, Direction::UP, 3, cn, cback);
                _this->_display->draw_arrow(415, 14, Direction::UP, 3, cn, cback);
            }
            xSemaphoreGive(_this->_display_mutex);
        }
        vTaskDelay(10);
    }
}

/**
 * @brief Writes a status message to the display
 * @param format - the format string
 * @param ... - variable argument list  
 */
void Controller::write_status_message(const String &format, ...)
{
    va_list args, copy;
    if(this->_display != nullptr)
    { 
        if (xSemaphoreTake(this->_display_mutex, portMAX_DELAY) == pdTRUE) 
        {
            va_list args, copy; 
            va_start(args, format); 
            va_copy(copy, args);
            // Determine the size of the formatted string 
      
            int len = vsnprintf(nullptr, 0, format.c_str(), args);
            if(len < 0) {
                // error condition, most likely in the format string. 
                va_end(args);
                va_end(copy);
                return;
            };

            // allocate memory for the operation
            char *buf = (char*) malloc(len+1);
            if(buf == NULL) {
                // memory allocation error.
                va_end(args); 
                va_end(copy);
                return;
            }            

            vsnprintf(buf, len+1, format.c_str(), args); 
            va_end(args); 
            va_end(copy);

            _display->write_status("%s", buf);
            free(buf);
            xSemaphoreGive(this->_display_mutex);
        }
    } 
};

/**
 * @brief Formats a string, essentially a wrapper for vnsprintf
 * @param format - format string
 * @param ... - variable argument list 
 */
String Controller::format_string(const char* format, ...) 
{
    va_list args; 
    va_start(args, format); 
    
    // Determine the size of the formatted string 
    size_t size = vsnprintf(nullptr, 0, format, args) + 1; // Add space for null terminator 
    va_end(args); 
    
    // Create a buffer of the appropriate size
    std::vector<char> buffer(size); 
    
    // Format the string 
    va_start(args, format); 
    vsnprintf(buffer.data(), size, format, args); 
    va_end(args); 
    return String(buffer.data());
}

/**
 * @brief Task function managing wheel movements. This task runs an endless blocking loop,
 * waiting for notification from handle_encoder_change upon which it will process
 * and execute the appropriate action.
 * @param args - pointer to task arguments 
 */
void Controller::input_runner(void* args)
{
    Wheel *_this = reinterpret_cast<Wheel *>(args);
    for (;;) 
    { 
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // this section is executed for every wheel position change.
        // To tranlsate into the CNC command, we need to use feed and axis
        switch(_this->_selected_axis)
        {
            case Axis::X:
                _this->_x += _this->_selected_feed * _this->_direction;
                break;
            case Axis::Y:
                _this->_y += _this->_selected_feed * _this->_direction;
                break;
            case Axis::Z:
                _this->_z += _this->_selected_feed * _this->_direction;
                break;             
        }
        
        if(!_this->_has_emergency)
        {
            String s = _this->format_string("G21G91%c%c%fF2000", 
                (char)_this->_selected_axis,
                _this->_direction == -1 ? '-': '+' ,
                _this->_selected_feed);
            Serial.println(s);
        }
    }
}

/**
 * @brief Event handler monitoring the Axus GPIOs 
 */
void IRAM_ATTR Controller::handle_axis_change()
{    
    if(!digitalRead(AXIS_X)) _selected_axis = Axis::X;
    if(!digitalRead(AXIS_Y)) _selected_axis = Axis::Y;
    else if(!digitalRead(AXIS_Z)) _selected_axis = Axis::Z;
    _direction = 0;
}

/**
 * @brief Event handler watching the Quadradure encoder GPIOs.
 */
void IRAM_ATTR Controller::handle_encoder_change()
{
    static int8_t c = 0;
    if(_has_emergency) return;

    static const int8_t enconder_state_table[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
    int MSB = digitalRead(WHEEL_A); // Most significant bit 
    int LSB = digitalRead(WHEEL_B); // Least significant bit 
    int encoded = (MSB << 1) | LSB; // Combine the two signals 
    if(encoded != _wheel_encoded)
    {
        int sum = (_wheel_encoded << 2) | encoded;  // Add the two previous bits 
        c += enconder_state_table[sum];
        if(c == 4 || c == -4)
        {
            _wheel_position += c == 4 ? 1 : -1;
            _direction = c > 0 ? 1 : -1;
            c = 0x0;
            _wheel_encoded = encoded;   // Update the last encoded value

            // Signal our job to run the axis....
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            vTaskNotifyGiveFromISR(_instance->_wheelRunner, &xHigherPriorityTaskWoken); 
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        else
        {
            _wheel_encoded = encoded;   // Update the last encoded value  
        }
    }
}