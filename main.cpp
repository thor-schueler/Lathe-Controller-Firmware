// Copyright (c) theRealThor. All rights reserved.
// SPDX-License-Identifier: MIT

/*
 * This is the firmware for teh HF 7x10 Minilathe controller with TFT display. 
 *   
 */

#define HIGH_WATER_MARK_LOOP_SKIP 120
#define BAUD_RATE 115200

#include "Arduino.h"
#include "ESP.h"
#include <SPI.h>

// C99 libraries
#include <cstdlib>
#include <string.h>
#include <time.h>

// local libraries
#include "src/logging/SerialLogger.h"
#include "src/controller/controller.h"

#define VERSION "0.00.00"
#define TELEMETRY_FREQUENCY_MILLISECS 120000


Controller *controller = NULL;

#ifdef SET_LOOP_TASK_STACK_SIZE
SET_LOOP_TASK_STACK_SIZE(16384);
  //
  // This will only work with ESP-Arduino 2.0.7 or higher. 
  //
#endif

/**
 * @brief Performs system setup activities, including connecting to WIFI, setting time, obtaining the IoTHub info 
 * from DPS if necessary and connecting the IoTHub. Use this method to also register various delegates and command
 * handlers.
 * 
 */
void setup()
{
  //
  // Initialize configuration data from EEPROM
  //
  Logger.SetSpeed(BAUD_RATE);

  Logger.Info_f(F("Copyright 2025, Thor Schueler, Firmware Version: %s"), VERSION);
  Logger.Info_f(F("Loop task stack size: %i"), getArduinoLoopTaskStackSize());
  Logger.Info_f(F("Loop task stack high water mark: %i"), uxTaskGetStackHighWaterMark(NULL));
  Logger.Info_f(F("Total heap: %d"), ESP.getHeapSize()); 
  Logger.Info_f(F("Free heap: %d"), ESP.getFreeHeap()); 
  Logger.Info_f(F("Total PSRAM: %d"), ESP.getPsramSize()); 
  Logger.Info_f(F("Free PSRAM: %d"), ESP.getFreePsram());
  Logger.Info(F("... Startup"));

  controller = new Controller();

  Logger.Info(F("... Init done"));
  Logger.Info_f(F("Free heap: %d"), ESP.getFreeHeap()); 
}

/**
 * @brief Main loop. Use this loop to execute recurring tasks. In this sample, we will periodically send telemetry
 * and query and update the device twin.  
 * 
 */
void loop()
{
  vTaskDelay(1000);
}
