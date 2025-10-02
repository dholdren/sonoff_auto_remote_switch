/*
 * Logger Class Implementation
 * For SONOFF S31 ESP8266 Project
 */

#include "Logger.h"
#include <stdarg.h>

// Static member initialization
WebSocketsServer* Logger::webSocket = nullptr;
bool Logger::webSocketEnabled = false;
bool Logger::serialEnabled = true;
char Logger::buffer[512];

// Global logger instance
Logger logger;

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
  if (serialEnabled && Serial) {
    if (addNewline) {
      Serial.println(message);
    } else {
      Serial.print(message);
    }
  }
  
  // Send to WebSocket if available and enabled
  if (webSocketEnabled && webSocket != nullptr) {
    String wsMessage = message;
    
    // Remove trailing newlines for JSON
    while (wsMessage.endsWith("\n") || wsMessage.endsWith("\r")) {
      wsMessage = wsMessage.substring(0, wsMessage.length() - 1);
    }
    
    // Escape special characters for JSON
    String escapedMessage = "";
    escapedMessage.reserve(wsMessage.length() * 2); // Reserve space for potential escaping
    
    for (int i = 0; i < wsMessage.length(); i++) {
      char c = wsMessage.charAt(i);
      switch (c) {
        case '\\':
          escapedMessage += "\\\\";
          break;
        case '"':
          escapedMessage += "\\\"";
          break;
        case '\b':
          escapedMessage += "\\b";
          break;
        case '\f':
          escapedMessage += "\\f";
          break;
        case '\n':
          escapedMessage += "\\n";
          break;
        case '\r':
          escapedMessage += "\\r";
          break;
        case '\t':
          escapedMessage += "\\t";
          break;
        default:
          // Handle other control characters
          if (c < 0x20) {
            char hexBuf[7];
            sprintf(hexBuf, "\\u%04x", (unsigned char)c);
            escapedMessage += hexBuf;
          } else {
            escapedMessage += c;
          }
          break;
      }
    }
    
    // Create JSON message for the browser
    String jsonMessage = "{\"type\":\"log\",\"message\":\"" + escapedMessage + "\"}";
    webSocket->broadcastTXT(jsonMessage);
  }
}