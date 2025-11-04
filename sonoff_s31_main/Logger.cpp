/*
 * Logger Class Implementation
 * For SONOFF S31 ESP8266 Project
 */

#include "config.h"
#include "Logger.h"
#include <stdarg.h>
#include "Adafruit_MQTT_Client.h"

// Static member initialization
Adafruit_MQTT_Publish* Logger::mqttLogger = nullptr;
bool Logger::serialEnabled = true;
char Logger::buffer[512];

// Global logger instance
Logger logger;

// externs
extern String UNIQUE_ID;
extern Adafruit_MQTT_Client mqtt;

void Logger::disableSerial() {
  serialEnabled = false;
}
void Logger::enableSerial() {
  serialEnabled = true;
}

void Logger::withoutSerial(void (*f_ptr)()) {
  bool oldSerialEnabled = serialEnabled;
  serialEnabled = false;
  f_ptr();
  serialEnabled = oldSerialEnabled;
}

void Logger::setMQTTLogger(Adafruit_MQTT_Publish* mp) {
  mqttLogger = mp;
}

void Logger::println(const String& message) {
  sendMessage(message, true);
}

void Logger::println(const char* message) {
  sendMessage(String(message), true);
}

void Logger::print(const String& message) {
  sendMessage(message, false);
}

void Logger::print(const char* message) {
  sendMessage(String(message), false);
}

void Logger::printf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  
  // Format the string into our buffer
  vsnprintf(buffer, sizeof(buffer), format, args);
  
  va_end(args);
  
  sendMessage(String(buffer), false);
}

void Logger::sendMessage(const String& message, bool addNewline) {
  if (serialEnabled && Serial) {
    if (addNewline) {
      Serial.println(message);
    } else {
      Serial.print(message);
    }
  }

  // Send to mqtt if available
  if (MQTT_LOGGING_ENABLED && mqttLogger != nullptr && mqtt.connected()) {
    mqttLogger->publish((UNIQUE_ID + " : " + message).c_str());
  }
}