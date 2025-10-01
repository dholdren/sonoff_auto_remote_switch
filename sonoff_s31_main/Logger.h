/*
 * Logger Class for SONOFF S31 ESP8266 Project
 * Provides dual output to Serial and WebSocket for browser debugging
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

class Logger {
private:
  static WebSocketsServer* webSocket;
  static bool webSocketEnabled;
  static char buffer[512]; // Buffer for formatted strings
  
public:
  // Initialize the logger with optional WebSocket server
  static void begin(WebSocketsServer* ws = nullptr);
  
  // Print functions
  static void println(const String& message);
  static void println(const char* message);
  static void print(const String& message);
  static void print(const char* message);
  
  // Printf-style functions
  static void printf(const char* format, ...);
  
  // WebSocket management
  static void setWebSocket(WebSocketsServer* ws);
  static void enableWebSocket(bool enable = true);
  
private:
  // Internal helper to send to both Serial and WebSocket
  static void sendMessage(const String& message, bool addNewline = false);
};

// Global logger instance for easy access
extern Logger logger;

#endif // LOGGER_H