/*
 * Logger Class Implementation
 * For SONOFF S31 ESP8266 Project
 */

#include "Logger.h"
#include <stdarg.h>

// Static member initialization
WebSocketsServer* Logger::webSocket = nullptr;
bool Logger::webSocketEnabled = false;
char Logger::buffer[512];

// Global logger instance
Logger logger;

void Logger::begin(WebSocketsServer* ws) {
  webSocket = ws;
  webSocketEnabled = (ws != nullptr);
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

void Logger::setWebSocket(WebSocketsServer* ws) {
  webSocket = ws;
  webSocketEnabled = (ws != nullptr);
}

void Logger::enableWebSocket(bool enable) {
  webSocketEnabled = enable && (webSocket != nullptr);
}

void Logger::sendMessage(const String& message, bool addNewline) {
  // Always send to Serial
  if (addNewline) {
    Serial.println(message);
  } else {
    Serial.print(message);
  }
  
  // Send to WebSocket if available and enabled
  if (webSocketEnabled && webSocket != nullptr) {
    String wsMessage = message;
    if (addNewline && !message.endsWith("\n")) {
      wsMessage += "\n";
    }
    
    // Create JSON message for the browser
    String jsonMessage = "{\"type\":\"log\",\"message\":\"" + wsMessage + "\"}";
    webSocket->broadcastTXT(jsonMessage);
  }
}