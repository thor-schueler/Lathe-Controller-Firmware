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
extern "C" {
  #include <driver/timer.h>
  #include <soc/gpio_struct.h>
}


portMUX_TYPE _hall_mux = portMUX_INITIALIZER_UNLOCKED;
static Controller *_instance = nullptr;

/**
 * @brief Creates a new instance of Controller
 */
Controller::Controller()
{
    Logger.Info(F("Startup"));
    Logger.Info(F("....Initialize Display"));
    _display = new Controller_Display();
    _display->init();
    _instance = this;

    Logger.Info(F("....Inititialize GPIO pins"));
    pinMode(I_MAIN_POWER, INPUT_PULLUP);
    pinMode(I_EMS, INPUT);
    pinMode(I_ENERGIZE, INPUT);
    pinMode(I_FOR_F, INPUT);
    pinMode(I_FOR_B, INPUT);
    pinMode(I_LIGHT, INPUT_PULLUP);
    pinMode(I_BACKLIGHT, INPUT_PULLDOWN); 
    pinMode(I_LUBE, INPUT_PULLUP);
    pinMode(I_CONTROLBOARD_DETECT, INPUT_PULLDOWN);
    pinMode(I_SPINDLE_PULSE, INPUT_PULLUP);


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
    attachInterrupt(digitalPinToInterrupt(I_LUBE), std::bind(&Controller::handle_input, this), CHANGE);
    attachInterrupt(digitalPinToInterrupt(I_BACKLIGHT), std::bind(&Controller::handle_input, this), CHANGE);
    attachInterrupt(digitalPinToInterrupt(I_ENERGIZE), std::bind(&Controller::handle_energize, this), FALLING);    
    attachInterrupt(digitalPinToInterrupt(I_CONTROLBOARD_DETECT), std::bind(&Controller::handle_input, this), RISING);
        // we only process rising (the signal is inverted) here as an interrupt as the control board might shut off 
        // due to overload and we need to be informed of that. All falling is initiated by us, so 
        // we do not need an interrupt for that. 

    Logger.Info("....Initializing counter timer");    
    timer_config_t cnt_config = 
    {
        .alarm_en = TIMER_ALARM_DIS,
        .counter_en = TIMER_PAUSE,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = TIMER_AUTORELOAD_DIS,
        .divider = TIMER_DIVIDER,
    };
    timer_init(TIMER_GROUP, TIMER_COUNTER, &cnt_config);
    timer_set_counter_value(TIMER_GROUP, TIMER_COUNTER, 0);
    timer_start(TIMER_GROUP, TIMER_COUNTER);

    if(USE_POLLING_FOR_RPM)
    {
        Logger.Info(F("     Not using interrupt for RPM sensing. Instead create RPM sample timer"));
        timer_config_t rpm_config = 
        {
            .alarm_en = TIMER_ALARM_EN,
            .counter_en = TIMER_PAUSE,
            .counter_dir = TIMER_COUNT_UP,
            .auto_reload = TIMER_AUTORELOAD_EN,
            .divider = TIMER_DIVIDER,
        };
        timer_init(TIMER_GROUP, TIMER_RPM, &rpm_config);
        timer_set_counter_value(TIMER_GROUP, TIMER_RPM, 0);
        timer_set_alarm_value(TIMER_GROUP, TIMER_RPM, HALL_POLLING_INTERVAL_US);
        timer_enable_intr(TIMER_GROUP, TIMER_RPM);
        timer_isr_callback_add(TIMER_GROUP, TIMER_RPM, Controller::read_hall_sensor, this, ESP_INTR_FLAG_IRAM);
        timer_start(TIMER_GROUP, TIMER_RPM);
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
    _toggle_energize = !digitalRead(I_ENERGIZE);
    _for_f = digitalRead(I_FOR_F);
    _for_b = digitalRead(I_FOR_B);
    _light = digitalRead(I_LIGHT);
    _backlight = digitalRead(I_BACKLIGHT);
    _lube = digitalRead(I_LUBE);
    _is_energized = !digitalRead(I_CONTROLBOARD_DETECT);
    if(_for_f)
    {
        this->_direction_a.desired = false;
        this->_direction_b.desired = false;
        this->_common.desired = true;
    }
    else if(_for_b)
    {
        this->_direction_a.desired = true;
        this->_direction_b.desired = true;
        this->_common.desired = true;
    }
    else
    {
        this->_direction_a.desired = false;
        this->_direction_b.desired = false;
        this->_common.desired = false;        
    }
    Logger.Info_f(F("         Main Power: %s"), _main_power ? "Off" : "On");
    Logger.Info_f(F("         Energize Toggled: %s"), _toggle_energize ? "Yes" : "No");
    Logger.Info_f(F("         Emergency Shutdown: %s"), _has_emergency ? "On" : "Off");
    Logger.Info_f(F("         Forward Selector: %s"), _for_f ? "On" : "Off");      
    Logger.Info_f(F("         Backward Selector: %s"), _for_b ? "On" : "Off");    
    Logger.Info_f(F("         Light: %s"), _light ? "Off" : "On");
    Logger.Info_f(F("         Backlight: %s"), _backlight ? "On" : "Off");  
    Logger.Info_f(F("         Lube: %s"), _lube ? "Off" : "On");
    Logger.Info_f(F("         Energized: %s"), _is_energized ? "HOT" : "COLD");

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

    Logger.Info(F("     Remove Counter timer"));
    timer_pause(TIMER_GROUP, TIMER_COUNTER);
    timer_set_counter_value(TIMER_GROUP, TIMER_COUNTER, 0);

    if(USE_POLLING_FOR_RPM)
    {
        Logger.Info(F("     Remove RPM timer"));
        timer_pause(TIMER_GROUP, TIMER_RPM);
        timer_disable_intr(TIMER_GROUP, TIMER_RPM);
        timer_set_counter_value(TIMER_GROUP, TIMER_RPM, 0);
        timer_set_alarm_value(TIMER_GROUP, TIMER_RPM, 0);
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
    detachInterrupt(digitalPinToInterrupt(I_BACKLIGHT));
    detachInterrupt(digitalPinToInterrupt(I_LUBE));
    detachInterrupt(digitalPinToInterrupt(I_CONTROLBOARD_DETECT));
}


/**
 * @brief Task function managing the display
 * @param args - pointer to task arguments
 */
void Controller::display_runner(void* args)
{
    Controller *_this = reinterpret_cast<Controller *>(args);
    unsigned int rpm = 0;
    word state = 0b1000000000000000;
        //         |      ||||||||+---- had_emergency
        //         |      |||||||+----- power state
        //         |      ||||||+------ engine energized state
        //         |      |||||+------- for_f state
        //         |      ||||+-------- for_b state
        //         |      |||+--------- light state
        //         |      ||+---------- backlight state
        //         |      |+----------- lube state 
        //         |      +------------ has deferred action
        //         +------------------- is first

    for (;;) 
    { 
        if (xSemaphoreTake(_this->_display_mutex, portMAX_DELAY) == pdTRUE) 
        {
            if(_this->_has_emergency)
            {
                // draw emergency shutdown screen
                _this->_display->write_emergency();
                state |= (1 << 0);
            }
            else
            {
                if(((state >> 0) & 0x1) || ((state >> 15) & 0x1))
                {
                    //restore display background after emergency
                    _this->_display->update_background();
                    state &= ~(1 << 0);
                    state |= (1 << 15);
                }

                if(_this->_rpm != rpm || ((state >> 15) & 0x1))
                {
                    // write RPMs to display
                    _this->_display->write_rpm(_this->_rpm);
                    rpm = _this->_rpm;
                }

                if(((state >> 1) & 0x1) != _this->_main_power || ((state >> 2) & 0x1) != _this->_is_energized || ((state >> 15) & 0x1))
                {
                    // update engine energized state
                    _this->_display->update_engine_state(_this->_is_energized);

                    // update power state
                    _this->_display->update_power_state(_this->_main_power);
                
                    state = (state & ~(1 << 1)) | (_this->_main_power << 1);
                    state = (state & ~(1 << 2)) | (_this->_is_energized << 2);
                }

                if(((state >> 3) & 0x1) != _this->_for_f || ((state >> 4) & 0x1) != _this->_for_b || ((state >> 15) & 0x1))
                {
                    // update FOR state
                    _this->_display->update_for_state(_this->_for_f, _this->_for_b);
                    state = (state & ~(1 << 3)) | (_this->_for_f << 3);
                    state = (state & ~(1 << 4)) | (_this->_for_b << 4);
                }

                if(((state >> 5) & 0x1) != _this->_light || ((state >> 15) & 0x1))
                {
                    // update light state
                    _this->_display->update_light_state(_this->_light);
                    state = (state & ~(1 << 5)) | (_this->_light << 5);
                }
          
                if(((state >> 6) & 0x1) != _this->_backlight || ((state >> 15) & 0x1))
                {
                    // update backlight state
                    _this->_display->update_back_light(_this->_backlight);
                    state = (state & ~(1 << 6)) | (_this->_backlight << 6);
                }

                if(((state >> 7) & 0x1) != _this->_lube || ((state >> 15) & 0x1))
                {
                    // update lube state
                    _this->_display->update_lube_state(_this->_lube);
                    state = (state & ~(1 << 7)) | (_this->_lube << 7);
                }

                if(((state >> 8) & 0x1) != _this->_has_deferred_action)
                {
                    // update warning area
                    _this->_display->update_warning(_this->_has_deferred_action);
                    state = (state & ~(1 << 8)) | (_this->_has_deferred_action << 8);
                }
            }
            xSemaphoreGive(_this->_display_mutex);
        }
        state &= ~(1 << 15);
        vTaskDelay(DISPLAY_REFRESH);
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
    static int index = 0;
    bool external_power_loss = false;
    Controller *_this = reinterpret_cast<Controller *>(args);
    for (;;) 
    { 
        bool should_print = false;

        //
        // read input states
        //
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        _this->_direction_a.reported = digitalRead(O_SPINDLE_DIRECTION_SWITCH_A);
        _this->_direction_b.reported = digitalRead(O_SPINDLE_DIRECTION_SWITCH_B);
        _this->_common.reported = digitalRead(O_SPINDLE_OFF);
        _this->_deenergize.reported = digitalRead(O_ENGINE_DISCHARGE);
        if(digitalRead(I_MAIN_POWER) != _this->_main_power) 
        {
            _this->_main_power = !_this->_main_power;
            should_print = true;
            Logger.Info_f(F("Main Power changed to: %s"), _this->_main_power ? "Off" : "On");
        }
        if(digitalRead(I_EMS) != _this->_has_emergency) 
        {
            _this->_has_emergency = !_this->_has_emergency;
            should_print = true;
            Logger.Info_f(F("EMS changed to: %s"), _this->_has_emergency ? "Shutdown" : "Energize");
        }
        if(digitalRead(I_LIGHT) != _this->_light) 
        {
            _this->_light = !_this->_light;
            Logger.Info_f(F("Light toggled: %s"), _this->_light ? "Off" : "On");
        }
        if(digitalRead(I_BACKLIGHT) != _this->_backlight) 
        {
            _this->_backlight = !_this->_backlight;
            Logger.Info_f(F("Backlight toggled: %s"), _this->_backlight ? "On" : "Off");
        }
        if(digitalRead(I_LUBE) != _this->_lube) 
        {
            _this->_lube = !_this->_lube;
            Logger.Info_f(F("Lubrication toggled: %s"), _this->_lube ? "Off" : "On");
        }
        if(digitalRead(I_FOR_F))
        {
            _this->_for_f = true;
            _this->_for_b = false;
            _this->_direction_a.desired = false;
            _this->_direction_b.desired = false;
            _this->_common.desired = true;
            if(_this->_direction_a.reported != false || _this->_direction_b.reported != false || _this->_common.reported != true)
            {
                should_print = true;
                Logger.Info(F("Direction changed to: Forward"));
            }
        }
        else if(digitalRead(I_FOR_B))
        {
            _this->_for_f = false;
            _this->_for_b = true;
            _this->_direction_a.desired = true;
            _this->_direction_b.desired = true;
            _this->_common.desired = true;
            if(_this->_direction_a.reported != true || _this->_direction_b.reported != true || _this->_common.reported != true)
            {
                should_print = true;
                Logger.Info(F("Direction changed to: Backward"));
            }
        }
        else
        {
            _this->_for_f = false;
            _this->_for_b = false;
            _this->_direction_a.desired = false;
            _this->_direction_b.desired = false;
            _this->_common.desired = false;
            if(_this->_direction_a.reported != false || _this->_direction_b.reported != false || _this->_common.reported != false)
            {
                should_print = true;
                Logger.Info(F("Direction changed to: Neutral"));
            }
        }
        if(digitalRead(I_CONTROLBOARD_DETECT) == HIGH && !_this->_toggle_energize && _this->_is_energized)
        {
            // if we read high here, than there is no voltage on the control board. If this happens without 
            // _toggle_energize being true while _is_energized, then the board has shutdown power basd on current or voltage
            // draw. So we need to "fake" de-energizing...  
            external_power_loss = true;
            Logger.Info(F("Received control board power loss trigger: Shutting down..."));
        }

        //
        // take appropriate action
        //
        _this->_common.desired = (!_this->_for_b && !_this->_for_f) ? false : true;
        if(!_this->_has_emergency)
        {
            if(_this->_toggle_energize)
            {
                _this->_toggle_energize = false;
                if(!_this->_main_power)
                {
                    unsigned int loop_break_counter = 0;
                    unsigned int stabilization_counter = 0;
                    byte reg = 0x0;

                    gpio_intr_disable(static_cast<gpio_num_t>(I_ENERGIZE));
                        // disable the interrupt during processing as there might be a lot of noise 
                        // coming on that pin during startup/shutdown.

                    do{
                        bool val = !digitalRead(I_ENERGIZE);            // I_ENERGIZE reads low if the button us pressed
                        reg = reg << 1;                                 // left shift register
                        reg |= val ?  1 << 0 : 0 << 0;                  // set least signifcant bit based on read value
                        stabilization_counter++;
                        vTaskDelay(pdMS_TO_TICKS(20));              
                    }
                    while(reg != 0xff && stabilization_counter < 40);
                        // delay any action until the button press on Energize is released
                        // we wait for eight subsequent readings of High before we move on 
                        // do make sure we are not being hit by bounce.  
                    if(stabilization_counter < 40)
                    {
                        if(_this->_is_energized)
                        {
                            Logger.Info_f("    De-Energizing engine...");
                            gpio_intr_disable(static_cast<gpio_num_t>(I_CONTROLBOARD_DETECT));  
                                // temporarily disable interrupt to prevent double processing
                            digitalWrite(O_ENGINE_DISCHARGE, HIGH);
                            do { 
                                _this->_is_energized = !digitalRead(I_CONTROLBOARD_DETECT); 
                                loop_break_counter++;
                                vTaskDelay(10);
                            }
                            while (_this->_is_energized && loop_break_counter < 1000);
                            if(!_this->_is_energized) Logger.Info("    Engine is now de-energized.");
                            else
                            {
                                Logger.Error("    Engine was not de-energized after waiting for 10sec. Check engine.");
                            }
                            digitalWrite(O_ENGINE_DISCHARGE, LOW);
                            vTaskDelay(pdMS_TO_TICKS(250));
                            gpio_intr_enable(static_cast<gpio_num_t>(I_CONTROLBOARD_DETECT));
                                // re-enable interrupt
                        }
                        else
                        {
                            Logger.Info_f("    Energizing engine...");
                            do { 
                                // power on happens on the motor control board, we just wait until we read the voltage
                                _this->_is_energized = !digitalRead(I_CONTROLBOARD_DETECT);
                                loop_break_counter++;
                                vTaskDelay(10); 
                            }
                            while (!_this->_is_energized && loop_break_counter < 1000);
                            if(_this->_is_energized) Logger.Info("    Engine is now energized.");
                            else
                            {
                                Logger.Error("    Engine was not energized after waiting for 10sec. Check engine.");
                            }
                        }
                    }
                    else
                    {
                        Logger.Info(F("Could not obtain stable reading on I_ENERGIZE. Cancelling transaction without change."));
                        if(!_this->_is_energized && digitalRead(I_CONTROLBOARD_DETECT) == LOW)
                        {
                            digitalWrite(O_ENGINE_DISCHARGE, HIGH);
                            vTaskDelay(pdMS_TO_TICKS(250));
                            digitalWrite(O_ENGINE_DISCHARGE, LOW);
                        }
                    }
                    gpio_intr_enable(static_cast<gpio_num_t>(I_ENERGIZE));
                }
            }
        
            if(external_power_loss)
            {
                unsigned int loop_break_counter = 0;
                Logger.Info(F("Responding to control board power loss trigger."));
                do { 
                    _this->_is_energized = !digitalRead(I_CONTROLBOARD_DETECT); 
                    loop_break_counter++;
                    vTaskDelay(10);
                }
                while (_this->_is_energized && loop_break_counter < 1000);
                    // confirm control board is indeed de-energized.
                external_power_loss = false;
            }

            if(!_this->_is_energized)
            {
                gpio_intr_disable(static_cast<gpio_num_t>(I_ENERGIZE));  
                                // temporarily disable interrupt to prevent induction lead processing
                digitalWrite(O_ENGINE_DISCHARGE, LOW);        
                if(_this->_direction_a.desired != _this->_direction_a.reported) { digitalWrite(O_SPINDLE_DIRECTION_SWITCH_A, _this->_direction_a.desired); should_print = true; }
                if(_this->_direction_b.desired != _this->_direction_b.reported) { digitalWrite(O_SPINDLE_DIRECTION_SWITCH_B, _this->_direction_b.desired); should_print = true; }
                if(_this->_common.desired != _this->_common.reported) { digitalWrite(O_SPINDLE_OFF, _this->_common.desired); should_print = true; }
                _this->_direction_a.reported = digitalRead(O_SPINDLE_DIRECTION_SWITCH_A);
                _this->_direction_b.reported = digitalRead(O_SPINDLE_DIRECTION_SWITCH_B);
                _this->_common.reported = digitalRead(O_SPINDLE_OFF);
                _this->_deenergize.reported = digitalRead(O_ENGINE_DISCHARGE);
                _this->_has_deferred_action = false;
                vTaskDelay(pdMS_TO_TICKS(250));
                gpio_intr_enable(static_cast<gpio_num_t>(I_ENERGIZE));
            }
            else
            {
                if(_this->_direction_a.desired != _this->_direction_a.reported) _this->_has_deferred_action = true;
                if(_this->_direction_b.desired != _this->_direction_b.reported) _this->_has_deferred_action = true;
                if(_this->_common.desired != _this->_common.reported) _this->_has_deferred_action = true;
                if(_this->_has_deferred_action)
                {
                    Logger.Info(F("Deferring Direction change due to engine lockout. Change will take place next time spindle if off."));
                    should_print = false;
                }
            }    
        }
        else
        {
            Logger.Info(F("Emergceny Shutdown Mode"));
            digitalWrite(O_ENGINE_DISCHARGE, HIGH);
            vTaskDelay(1000);
            digitalWrite(O_SPINDLE_OFF, LOW);
        }
        
        _this->_direction_a.reported = digitalRead(O_SPINDLE_DIRECTION_SWITCH_A);
        _this->_direction_b.reported = digitalRead(O_SPINDLE_DIRECTION_SWITCH_B);
        _this->_common.reported = digitalRead(O_SPINDLE_OFF);
        _this->_deenergize.reported = digitalRead(O_ENGINE_DISCHARGE);
        if(should_print)
        {
            Logger.Info  (F("Status:"));
            Logger.Info_f(F("    Engine power: %s"), _this->_is_energized ? "Hot" : "Cold");
            Logger.Info_f(F("    Direction Relay A: %s"), _this->_direction_a.reported ? "Reverse" : "Forward");
            Logger.Info_f(F("    Direction Relay B: %s"), _this->_direction_b.reported ? "Reverse" : "Forward");
            Logger.Info_f(F("    Direction Relay Common: %s"), _this->_common.reported ? "Energized" : "Off");
            Logger.Info_f(F("    Denergize Relay: %s"), _this->_deenergize.reported ? "Open" : "Closed");
        }

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
    uint64_t now = 0;
    uint64_t validTimes[MAX_RPM_PULSES];
    uint64_t sum = 0;
    float avgDelta = 0.0;
    float rawRPM = 0.0;
    int validCount = 0;
    int count = (this->_pulse_count < MAX_RPM_PULSES) ? this->_pulse_count : MAX_RPM_PULSES;
    if (count < 2)
    { 
        // we need at least two pulses to calculate RPMs. 
        this->_rpm = 0;
        return;
    }    

    // Filter out timestamps older than MAX_RPM_AGE_US
    timer_get_counter_value(TIMER_GROUP, TIMER_COUNTER, &now);
    for (int i = 0; i < count; i++) 
    {
        if (now - this->_pulse_times[i] <= MAX_RPM_AGE_US) 
        {
            validTimes[validCount++] = this->_pulse_times[i];
        }
    }
    if (validCount < 2) 
    {
        // we need at least two pulses to calculate RPMs.
        this->_rpm = 0;
        return;
    }

    // Calculate average delta
    for (int i = 1; i < validCount; i++) sum += validTimes[i] - validTimes[i-1];

    avgDelta = sum / float(validCount - 1);
    rawRPM = 60000000.0 / avgDelta;

    // Jitter suppression
    if (RPM_SMOOTHING_ALPHA > 0.0 && RPM_SMOOTHING_ALPHA < 1.0 && abs((int)rawRPM - (int)this->_rpm) > MIN_RPM_DELTA)
    {
        this->_rpm = RPM_SMOOTHING_ALPHA * rawRPM + (1.0 - RPM_SMOOTHING_ALPHA) * this->_rpm;
            // Exponential smoothing
    }
    else if (abs((int)rawRPM - (int)this->_rpm) > MIN_RPM_DELTA) this->_rpm = rawRPM;
}

/**
 * @brief Interrupt handler to read the State of the Hall sensor to measure Motor RPM
 *
 * @param arg pointer to class instance context (this)
 */
bool IRAM_ATTR Controller::read_hall_sensor(void *arg) 
{
    static byte reg = 0x0;
    static bool last_stable_state = 1;

    // hall sensor will read logical 1 until the magnet gets close to the sensor, when it
    // switches to logical 0. So it is equivalent to a normally closed switch. So will
    // monitor for a stable low signal while our state is high.  

    Controller* _this = reinterpret_cast<Controller *>(arg);
    portENTER_CRITICAL_ISR(&_hall_mux);

    timer_group_clr_intr_status_in_isr(TIMER_GROUP, TIMER_RPM);
                                                // clear the timer interrupt to allow for subsequent processing
    bool val = 0;
    if(I_SPINDLE_PULSE <32) val = (GPIO.in >> I_SPINDLE_PULSE) & 0x1;
    else if (I_SPINDLE_PULSE < 40) val = (GPIO.in1.data >> (I_SPINDLE_PULSE - 32)) & 0x1;
    else val = 0;

    reg = ((reg << 1) | (val ? 1 : 0)) & 0x07;  // Shift in current value, keep last 3 bits
    bool stable_state = (reg == 0x07) ? 1 : (reg == 0x00) ? 0 : last_stable_state;
                                                // Determine stable state from 3-sample debounce

    // Detect falling edge: HIGH → LOW
    if (last_stable_state == 1 && stable_state == 0) 
    {
        uint64_t currentTime = 0;
        timer_get_counter_value(TIMER_GROUP, TIMER_COUNTER, &currentTime); 
            // Read hardware timer, this is a 64bit value, so it rolls over every
            // 584,942 years

        if (_this->_pulse_count < MAX_RPM_PULSES) _this->_pulse_times[_this->_pulse_count++] = currentTime;
        else 
        {
            // Shift left to discard oldest
            memmove((void*)&(_this->_pulse_times[0]), (void*)&(_this->_pulse_times[1]), sizeof(uint64_t) * (MAX_RPM_PULSES - 1));
            _this->_pulse_times[MAX_RPM_PULSES - 1] = currentTime;
            _this->_counter++;
        }
    }
    last_stable_state = stable_state;
    portEXIT_CRITICAL_ISR(&_hall_mux);
    return false;
}

/**
 * @brief Event handler watching for changes on the energize button toggle.
 */
void IRAM_ATTR Controller::handle_energize()
{
    static uint64_t last_toggle_energize = 0;
    uint64_t currentTime = 0;
    timer_get_counter_value(TIMER_GROUP, TIMER_COUNTER, &currentTime); 
        // Read hardware timer, this is a 64bit value, so it rolls over every
        // 584,942 years
    if (currentTime - last_toggle_energize > DEBOUNCE_US) 
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        last_toggle_energize = currentTime;
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
    static uint64_t last_input_change = 0;    
    uint64_t currentTime = 0;
    timer_get_counter_value(TIMER_GROUP, TIMER_COUNTER, &currentTime); 
        // Read hardware timer, this is a 64bit value, so it rolls over every
        // 584,942 years
    if (currentTime - last_input_change > DEBOUNCE_US) 
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        last_input_change = currentTime;
        vTaskNotifyGiveFromISR(this->_input_runner, &xHigherPriorityTaskWoken); 
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
 * @brief Event handler monitoring the Spindle Pulse
 */
void IRAM_ATTR Controller::handle_spindle_pulse()
{    
    static unsigned long hall_debounce_tick = 0;
    uint64_t now = 0;
    portENTER_CRITICAL_ISR(&_hall_mux);
    timer_get_counter_value(TIMER_GROUP, TIMER_COUNTER, &now); 
        // Read hardware timer, this is a 64bit value, so it rolls over every
        // 584,942 years
    if(now - hall_debounce_tick > HALL_DEBOUNCE_DELAY_US)
    {
        hall_debounce_tick = now;

        if (this->_pulse_count < MAX_RPM_PULSES) this->_pulse_times[this->_pulse_count++] = now;
        else 
        {
            // Shift left to discard oldest
            memmove((void*)&this->_pulse_times[0], (void*)&this->_pulse_times[1], sizeof(uint64_t) * (MAX_RPM_PULSES - 1));
            this->_pulse_times[MAX_RPM_PULSES - 1] = now;
        }
    }
    portEXIT_CRITICAL_ISR(&_hall_mux);
}
