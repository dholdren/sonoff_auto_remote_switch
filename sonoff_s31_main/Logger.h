/*
 * Logger Class for SONOFF S31 ESP8266 Project
 * Provides dual output to Serial and MQTT for remote debugging
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include "Adafruit_MQTT.h"

class Logger {
private:
  static Adafruit_MQTT_Publish* mqttLogger;
  static bool serialEnabled;
  static char buffer[512]; // Buffer for formatted strings

public:
  static void disableSerial();
  static void enableSerial();
  static void withoutSerial(void (*f_ptr)());
  
  // Print functions
  static void println(const String& message);
  static void println(const char* message);
  static void print(const String& message);
  static void print(const char* message);
  
  // Printf-style functions
  static void printf(const char* format, ...);
  
  // MQTT management
  static void setMQTTLogger(Adafruit_MQTT_Publish* mp);
  
private:
  // Internal helper to send to both Serial and MQTT
  static void sendMessage(const String& message, bool addNewline = false);
};

// Global logger instance for easy access
extern Logger logger;

#endif // LOGGER_H