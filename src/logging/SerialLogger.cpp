// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Thor Schueler. All rights reserved.
// SPDX-License-Identifier: MIT

#include <Arduino.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "SerialLogger.h"

#define UNIX_EPOCH_START_YEAR 1900

/**
 * @brief Construct a new Serial Logger:: Serial Logger object
 * 
 */
SerialLogger::SerialLogger() 
{ 
  Serial.begin(SERIAL_LOGGER_BAUD_RATE); 
}


#pragma region Information Logging methods
/**
 * @brief Logs an information message to the serial console. 
 * 
 * @param message The message to log
 */
void SerialLogger::Info(String message)
{
  Serial.print("; ");
  this->writeTime();
  Serial.print(F(" [INFO] "));
  Serial.println(message);
}

/**
 * @brief Logs a formatted message to the serial console. Follows print_f conventions.
 * 
 * @param format The format string
 * @param ... Argument list for the token replacement in the format string.
 * @return size_t The lenght of the actual string logged.
 */
size_t SerialLogger::Info_f(String format, ...)
{
  char *buf = NULL;
  va_list arg;
  va_list copy;
  Serial.print("; ");
  this->writeTime();
  Serial.print(F(" [INFO] "));

  va_start(arg, format);
  va_copy(copy, arg);
  int len = vsnprintf(NULL, 0, format.c_str(), arg);
    // determine the length first, before we attempt the actual conversion 
  if(len < 0) {
    // error condition, most likely in the format string. 
    va_end(arg);
    va_end(copy);
    return 0;
  };

  // allocate memory for the operation
  buf = (char*) malloc(len+1);
  if(buf == NULL) {
    // memory allocation error. 
    va_end(arg);
    va_end(copy);
    return 0;
  } 
  len = vsnprintf(buf, len+1, format.c_str(), copy);
  va_end(arg);
  va_end(copy);
  len = Serial.print(buf);
  free(buf);
  Serial.println();
  return len;
}
#pragma endregion 

#pragma region Error Logging methods
/**
 * @brief Logs an error message to the serial console. 
 * 
 * @param message The message to log
 */  
void SerialLogger::Error(String message)
{
  Serial.print("; ");
  this->writeTime();
  Serial.print(F(" [ERROR] "));
  Serial.println(message);
}

/**
 * @brief Logs a formatted error to the serial console. Follows print_f conventions.
 * 
 * @param format The format string
 * @param ... Argument list for the token replacement in the format string.
 * @return size_t The lenght of the actual string logged.
 */ 
size_t SerialLogger::Error_f(String format, ...)
{
  char *buf = NULL;
  va_list arg;
  va_list copy;
  Serial.print("; ");
  this->writeTime();
  Serial.print(F(" [ERROR] "));

  va_start(arg, format);
  va_copy(copy, arg);
  int len = vsnprintf(NULL, 0, format.c_str(), arg);
  if(len < 0) {
    // error condition, most likely in the format string. 
    va_end(arg);
    va_end(copy);
    return 0;
  };
  // allocate memory for the operation
  buf = (char*) malloc(len+1);
  if(buf == NULL) {
    // memory allocation error. 
    va_end(arg);
    va_end(copy);
    return 0;
  } 
  len = vsnprintf(buf, len+1, format.c_str(), copy);
  va_end(arg);
  va_end(copy);
  len = Serial.print(buf);
  free(buf);

  Serial.println();
  return len;
}
#pragma endregion

#pragma region private methods
/**
 * @brief Writes the current time inline to the console. 
 * 
 */
void SerialLogger::writeTime()
{
  struct tm* ptm;
  time_t now = time(NULL);

  ptm = localtime(&now);
  Serial.print(ptm->tm_year + UNIX_EPOCH_START_YEAR);
  Serial.print("/");
  Serial.print(ptm->tm_mon + 1);
  Serial.print("/");
  Serial.print(ptm->tm_mday);
  Serial.print(" ");
  
  if (ptm->tm_hour < 10)
  {
    Serial.print(0);
  }

  Serial.print(ptm->tm_hour);
  Serial.print(":");

  if (ptm->tm_min < 10)
  {
    Serial.print(0);
  }

  Serial.print(ptm->tm_min);
  Serial.print(":");

  if (ptm->tm_sec < 10)
  {
    Serial.print(0);
  }

  Serial.print(ptm->tm_sec);
}
#pragma endregion

/**
 * @brief Sets the transmission speed
 * @param speed - the transmission speed. 
 */
void SerialLogger::SetSpeed(uint32_t speed)
{
  if(Serial)
  {
    Serial.flush();
    Serial.end();
  }
  Serial.begin(speed);
}

/**
 * @brief Global instance to be used for logging
 * 
 */
SerialLogger Logger;
