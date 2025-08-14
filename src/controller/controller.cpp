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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "controller.h"
#include "../logging/SerialLogger.h"
#include <portmacro.h>

portMUX_TYPE _hall_mux = portMUX_INITIALIZER_UNLOCKED;
static Controller *_instance = nullptr;

/**
 * @brief Creates a new instance of Controller
 */
Controller::Controller()
{
    vTaskDelay(5000);
    Logger.Info(F("Startup"));
    Logger.Info(F("....Initialize Display"));
    _display = new Controller_Display();
    _display->set_rotation(1);
    _display->init();
    _instance = this;

    Logger.Info(F("....Inititialize GPIO pins"));
    pinMode(I_MAIN_POWER, INPUT_PULLUP);
    pinMode(I_EMS, INPUT);
    pinMode(I_ENERGIZE, INPUT);
    pinMode(I_FOR_F, INPUT);
    pinMode(I_FOR_B, INPUT);
    pinMode(I_LIGHT, INPUT_PULLUP);
    pinMode(I_CONTROLBOARD_DETECT, INPUT_PULLDOWN);
    pinMode(I_SPINDLE_PULSE, INPUT_PULLDOWN);

    pinMode(O_SPINDLE_DIRECTION_SWITCH_A, OUTPUT);
    pinMode(O_SPINDLE_DIRECTION_SWITCH_B, OUTPUT);
    pinMode(O_SPINDLE_OFF, OUTPUT);
    pinMode(O_ENGINE_DISCHARGE, OUTPUT);


    Logger.Info(F("....Attach event receivers for GPIO"));
    attachInterrupt(digitalPinToInterrupt(I_MAIN_POWER), std::bind(&Controller::handle_input, this), CHANGE);
    attachInterrupt(digitalPinToInterrupt(I_EMS), std::bind(&Controller::handle_input, this), CHANGE);
    attachInterrupt(digitalPinToInterrupt(I_FOR_F), std::bind(&Controller::handle_input, this), CHANGE);
    attachInterrupt(digitalPinToInterrupt(I_FOR_B), std::bind(&Controller::handle_input, this), CHANGE);
    attachInterrupt(digitalPinToInterrupt(I_LIGHT), std::bind(&Controller::handle_input, this), CHANGE);
    attachInterrupt(digitalPinToInterrupt(I_ENERGIZE), std::bind(&Controller::handle_energize, this), FALLING);
    //attachInterrupt(digitalPinToInterrupt(I_CONTROLBOARD_DETECT), std::bind(&Controller::handle_input, this), CHANGE);

    if(USE_POLLING_FOR_RPM)
    {
        Logger.Info(F("     Not using interrupt for RPM sensing. Instead create RPM sample timer"));
        const esp_timer_create_args_t ta = {
            .callback = read_hall_sensor,
            .arg = this,
            .name = "rmp sensing",
            .skip_unhandled_events = true
        }; 
        esp_timer_create(&ta, &this->_hall_timer_handle);
        esp_timer_start_periodic(this->_hall_timer_handle, HALL_POLLING_INTERVAL_US);
    }
    else
    {
        Logger.Info_f(F("     Register Interrupt Handler for Hall Sensor on pin %i"), I_SPINDLE_PULSE);
        attachInterrupt(digitalPinToInterrupt(I_SPINDLE_PULSE), std::bind(&Controller::handle_spindle_pulse, this), FALLING);
    }
    

    Logger.Info("....Initializing Relays");
    digitalWrite(O_ENGINE_DISCHARGE, LOW); 
    digitalWrite(O_SPINDLE_OFF, LOW);
    digitalWrite(O_SPINDLE_DIRECTION_SWITCH_A, LOW);
    digitalWrite(O_SPINDLE_DIRECTION_SWITCH_B, LOW);

    Logger.Info("....Initializing Input Values");  
    _main_power = digitalRead(I_MAIN_POWER);
    _has_emergency = digitalRead(I_EMS);
    _toggle_energize = digitalRead(I_ENERGIZE);
    _for_f = digitalRead(I_FOR_F);
    _for_b = digitalRead(I_FOR_B);
    _light = digitalRead(I_LIGHT);
    _is_energized = digitalRead(I_CONTROLBOARD_DETECT);
    Logger.Info_f(F("         Main Power: %s"), _main_power ? "Off" : "On");
    Logger.Info_f(F("         Emergency Shutdown: %s"), _has_emergency ? "On" : "Off");
    Logger.Info_f(F("         Forward Selector: %s"), _for_f ? "On" : "Off");      
    Logger.Info_f(F("         Backward Selector: %s"), _for_b ? "On" : "Off");    
    Logger.Info_f(F("         Light: %s"), _light ? "Off" : "On");
    Logger.Info_f(F("         Energized: %s"), _is_energized ? "Hot" : "Cold");  

    Logger.Info(F("....Generating Mutexes"));
    _display_mutex = xSemaphoreCreateBinary();  xSemaphoreGive(_display_mutex);

    Logger.Info("....Create various tasks");
    xTaskCreatePinnedToCore(display_runner, "displayRunner", 8192, this, 1, &_display_runner, 0);
    xTaskCreatePinnedToCore(input_runner, "inputRunner", 2048, this, configMAX_PRIORITIES-1, &_input_runner, 0);
    xTaskCreatePinnedToCore(rpm_runner, "rpmRunner", 2048, this, configMAX_PRIORITIES-1, &_rpm_runner, 0);

    Logger.Info("Startup done");
    Logger.Info("");
    Logger.Info("");
}

/**
 * @brief Cleans up resources
 */
Controller::~Controller()
{
    Logger.Info(F("Destruct controller client and clean up resources"));
    Logger.Info(F("     Remove tasks"));
    if(this->_display_runner != NULL) vTaskDelete(this->_display_runner);
    if(this->_input_runner != NULL) vTaskDelete(this->_input_runner);
    if(this->_rpm_runner != NULL) vTaskDelete(this->_rpm_runner);
    this->_display_runner = NULL;
    this->_input_runner = NULL;
    this->_rpm_runner = NULL;

    if(USE_POLLING_FOR_RPM)
    {
        Logger.Info(F("     Remove RPM timer"));
        if(this->_hall_timer_handle != NULL)
        {
            if(esp_timer_is_active(this->_hall_timer_handle)) esp_timer_stop(this->_hall_timer_handle);
            esp_timer_delete(this->_hall_timer_handle);
        }
    }
    else
    {
        Logger.Info(F("     Remove RPM hall sensor interrupt"));
        detachInterrupt(digitalPinToInterrupt(I_SPINDLE_PULSE));
    }

    Logger.Info(F("     Remove interrupts"));
    detachInterrupt(digitalPinToInterrupt(I_MAIN_POWER));
    detachInterrupt(digitalPinToInterrupt(I_EMS));
    detachInterrupt(digitalPinToInterrupt(I_FOR_F));
    detachInterrupt(digitalPinToInterrupt(I_FOR_B));
    detachInterrupt(digitalPinToInterrupt(I_LIGHT));
    detachInterrupt(digitalPinToInterrupt(I_ENERGIZE));
}


/**
 * @brief Task function managing the display
 * @param args - pointer to task arguments
 */
void Controller::display_runner(void* args)
{
    Controller *_this = reinterpret_cast<Controller *>(args);
    for (;;) 
    { 
        if (xSemaphoreTake(_this->_display_mutex, portMAX_DELAY) == pdTRUE) 
        {
            //int16_t cf = _this->_display->RGB_to_565(0x00, 0xff, 0x00);
            //int16_t cb = _this->_display->RGB_to_565(0xff, 0x00, 0x00);
            //int16_t cn = _this->_display->RGB_to_565(177,0,254);
            //int16_t cback = _this->_display->RGB_to_565(177,0,254);
            
            //_this->_display->write_axis(_this->_selected_axis);
            //_this->_display->write_feed(_this->_selected_feed);
            //_this->_display->write_emergency(_this->_has_emergency);
            //_this->_display->write_x(_this->_x);
            //_this->_display->write_y(_this->_y);
            //_this->_display->write_z(_this->_z);

            //if(_this->_direction == 1)
            //{
            //    _this->_display->draw_arrow(185, 14, Direction::RIGHT, 3, _this->_selected_axis == Axis::X ? cf : cn, cback);
            //    _this->_display->draw_arrow(305, 14, Direction::DOWN, 3, _this->_selected_axis == Axis::Y ? cb : cn, cback);
            //    _this->_display->draw_arrow(415, 14, Direction::UP, 3, _this->_selected_axis == Axis::Z ? cf : cn, cback);
            //}
            //else if(_this->_direction == -1)
            //{
            //    _this->_display->draw_arrow(185, 14, Direction::LEFT, 3, _this->_selected_axis == Axis::X ? cb : cn, cback);
            //    _this->_display->draw_arrow(305, 14, Direction::UP, 3, _this->_selected_axis == Axis::Y ? cf : cn, cback);
            //    _this->_display->draw_arrow(415, 14, Direction::DOWN, 3, _this->_selected_axis == Axis::Z ? cb : cn, cback);
            //}
            //else
            //{
            //    _this->_display->draw_arrow(185, 14, Direction::LEFT, 3, cn, cback);
            //    _this->_display->draw_arrow(305, 14, Direction::UP, 3, cn, cback);
            //    _this->_display->draw_arrow(415, 14, Direction::UP, 3, cn, cback);
            //}
            xSemaphoreGive(_this->_display_mutex);
        }
        vTaskDelay(10);
    }
}

/**
 * @brief Task function managing wheel movements. This task runs an endless blocking loop,
 * waiting for notification from handle_encoder_change upon which it will process
 * and execute the appropriate action.
 * @param args - pointer to task arguments 
 */
void Controller::input_runner(void* args)
{
    Controller *_this = reinterpret_cast<Controller *>(args);
    for (;;) 
    { 
        //
        // read input states
        //
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        Logger.Info_f(F("         Energized: %d - %d"), digitalRead(I_MAIN_POWER), _this->_main_power); 
        if(digitalRead(I_MAIN_POWER) != _this->_main_power) 
        {
            _this->_main_power = !_this->_main_power;
            Logger.Info_f(F("Main Power changed to: %s"), _this->_main_power ? "Off" : "On");
        }
        if(digitalRead(I_EMS) != _this->_has_emergency) 
        {
            _this->_has_emergency = !_this->_has_emergency;
            Logger.Info_f(F("EMS changed to: %s"), _this->_has_emergency ? "Shutdown" : "Energize");
        }
        if(digitalRead(I_LIGHT) != _this->_light) 
        {
            _this->_light = !_this->_light;
            Logger.Info_f(F("Light toggled: %s"), _this->_light ? "On" : "Off");
        }
        if(digitalRead(I_FOR_F) != _this->_for_f) 
        {
            _this->_for_f = !_this->_for_f;
             _this->_direction_a.desired = _this->_for_f;
            Logger.Info_f(F("Forward Direction changed to: %s"), _this->_for_f ? "On" : "Off");
        }
        if(digitalRead(I_FOR_B) != _this->_for_b) 
        {
            _this->_for_b = !_this->_for_b;
            _this->_direction_b.desired = _this->_for_b;
            Logger.Info_f(F("Backward Direction changed to: %s"), _this->_for_b ? "On" : "Off");
        }
        if(digitalRead(I_CONTROLBOARD_DETECT) != _this->_is_energized) 
        {
            _this->_is_energized = !_this->_is_energized;
            Logger.Info_f(F("Engine power: %s"), _this->_is_energized ? "Hot" : "Cold");
        }

        //
        // take appropriate action
        //
        _this->_common.desired = (!_this->_for_b && !_this->_for_f) ? false : true;
        if(!_this->_is_energized)
        {
            if(_this->_for_f)
            {
                digitalWrite(O_SPINDLE_DIRECTION_SWITCH_A, LOW);
                digitalWrite(O_SPINDLE_DIRECTION_SWITCH_B, LOW);
                _this->_direction_b.desired = false;
                _this->_for_b = false;
            }
            if(_this->_for_b)
            {
                digitalWrite(O_SPINDLE_DIRECTION_SWITCH_A, HIGH);
                digitalWrite(O_SPINDLE_DIRECTION_SWITCH_B, HIGH);
                _this->_direction_a.desired = false;
                _this->_for_f = false;
            }            
            if(_this->_common.desired != _this->_common.reported) digitalWrite(O_SPINDLE_OFF, HIGH);            
        }

        if(_this->_toggle_energize)
        {
            if(_this->_is_energized)
            {
                Logger.Info_f("     De-Energizing engine...");
                digitalWrite(O_SPINDLE_OFF, HIGH);
                do { 
                    _this->_is_energized = digitalRead(I_CONTROLBOARD_DETECT); 
                    vTaskDelay(10);
                }
                while (_this->_is_energized);
                digitalWrite(O_SPINDLE_OFF, LOW);
            }
            else
            {
                Logger.Info_f("     Energizing engine...");
                do { 
                    // power on happens on the motor control board, we just wait until we read the voltage
                    _this->_is_energized = digitalRead(I_CONTROLBOARD_DETECT);
                    vTaskDelay(10); 
                }
                while (_this->_is_energized);
            }
            _this->_toggle_energize = false;
        }
        
        _this->_direction_a.reported = digitalRead(O_SPINDLE_DIRECTION_SWITCH_A);
        _this->_direction_b.reported = digitalRead(O_SPINDLE_DIRECTION_SWITCH_B);
        _this->_common.reported = digitalRead(O_SPINDLE_OFF);
        _this->_deenergize.reported = digitalRead(O_ENGINE_DISCHARGE);
        Logger.Info  (F("Status:"));
        Logger.Info_f(F("    Engine power: %s"), _this->_is_energized ? "Hot" : "Cold");
        Logger.Info_f(F("    Direction Relay A: %s"), _this->_direction_a.reported ? "Forward" : "Backward");
        Logger.Info_f(F("    Direction Relay B: %s"), _this->_direction_b.reported ? "Forward" : "Backward");
        Logger.Info_f(F("    Direction Relay Common: %s"), _this->_common.reported ? "Closed" : "Open");
        Logger.Info_f(F("    Denergize Relay: %s"), _this->_deenergize.reported ? "Open" : "Closed");

        //
        // block execution until next event trigger
        //
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

/**
 * @brief Task function caluclating the spindle RPM based on the pulses on a regular schedule.
 * @param args - pointer to task arguments 
 */
void Controller::rpm_runner(void* args)
{
    Controller *_this = reinterpret_cast<Controller *>(args);
    for (;;) 
    { 
        _this->calculate_rpm();
        vTaskDelay(pdMS_TO_TICKS(RPM_CALCULATION_INTERVAL));
    }
}


/**
 * @brief Calculates the rpm based on the collected pulses
 */
void Controller::calculate_rpm() 
{
    uint32_t validTimes[MAX_RPM_PULSES];
    uint32_t sum = 0;
    float avgDelta = 0.0;
    float rawRPM = 0.0;
    int validCount = 0;
    int count = (this->_pulse_index < MAX_RPM_PULSES) ? this->_pulse_index : MAX_RPM_PULSES;
    if (count < 2) return;
        // we need at least two pulses to calculate RPMs. 

    // Filter out timestamps older than MAX_RPM_AGE_US
    for (int i = 0; i < count; i++) 
    {
        int idx = (this->_pulse_index - i - 1 + MAX_RPM_PULSES) % MAX_RPM_PULSES;
        if (micros() - this->_pulse_times[idx] <= MAX_RPM_AGE_US) validTimes[validCount++] = this->_pulse_times[idx];
    }
    if (validCount < 2) return;
        // we need at least two pulses to calculate RPMs.

    // Calculate average delta
    for (int i = 1; i < validCount; i++) sum += validTimes[i - 1] - validTimes[i];

    avgDelta = sum / float(validCount - 1);
    rawRPM = 60000000.0 / avgDelta;

    // Jitter suppression
    if (abs((int)rawRPM - (int)this->_rpm) > MIN_RPM_DELTA)
    {
        this->_rpm = RPM_SMOOTHING_ALPHA * rawRPM + (1.0 - RPM_SMOOTHING_ALPHA) * this->_rpm;
            // Exponential smoothing
    }
}

/**
 * @brief Interrupt handler to read the State of the Hall sensor to measure Motor RPM
 *
 * @param arg pointer to class instance context (this)
 */
void IRAM_ATTR Controller::read_hall_sensor(void *arg) 
{
    static byte reg = 0x0;
    static bool state = 0x1;

    // hall sensor will read logical 1 until the magnet gets close to the sensor, when it
    // switches to logical 0. So it is equivalent to a normally closed switch. So will
    // monitor for a stable low signal while our state is high.  

    Controller* _this = reinterpret_cast<Controller *>(arg);
    portENTER_CRITICAL_ISR(&_hall_mux);
    bool val = digitalRead(I_SPINDLE_PULSE);
    reg = reg << 1;                                 // left shift register
    reg |= val ?  1 << 0 : 0 << 0;                  // set least signifcant bit based on read value
    if(reg == 0xff && state == 0x0) state = 0x1;    // if all bits are set, we have a stable 1 state
    if(reg == 0x0 && state == 0x1)                  // if all bits are unset, we have a stable 0 state
    {                                               // if at the same time our state is 1, we have a state
                              // transition. 
          
        _this->_pulse_times[_this->_pulse_index % MAX_RPM_PULSES] = micros();
        _this->_pulse_index++;
        state = 0x0;
    }
    portEXIT_CRITICAL_ISR(&_hall_mux);
}

/**
 * @brief Event handler watching for changes on the energize button toggle.
 */
void IRAM_ATTR Controller::handle_energize()
{
    unsigned long currentTime = millis();
    if (currentTime - this->_last_toggle_energize > DEBOUNCE_MS) 
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        this->_last_toggle_energize = currentTime;
        this->_toggle_energize = true;
        vTaskNotifyGiveFromISR(this->_input_runner, &xHigherPriorityTaskWoken); 
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
 * @brief Event handler watching for changes on any inputs.
 */
void IRAM_ATTR Controller::handle_input()
{
    unsigned long currentTime = millis();
    if (currentTime - this->_last_input_change > DEBOUNCE_MS) 
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        this->_last_input_change = currentTime;
        vTaskNotifyGiveFromISR(this->_input_runner, &xHigherPriorityTaskWoken); 
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
 * @brief Event handler monitoring the Spindle Pulse
 */
void IRAM_ATTR Controller::handle_spindle_pulse()
{    
    portENTER_CRITICAL_ISR(&_hall_mux);
    uint64_t now = esp_timer_get_time();
    if(now - this->_hall_debounce_tick > HALL_DEBOUNCE_DELAY_US)
    {
        this->_hall_debounce_tick = now;
        this->_pulse_times[this->_pulse_index % MAX_RPM_PULSES] = micros();
        this->_pulse_index++;
    }
    portEXIT_CRITICAL_ISR(&_hall_mux);
}
