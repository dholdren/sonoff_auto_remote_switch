/*
 * Web Interface Handler
 * For SONOFF S31 ESP8266 Project
 */

#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>

// Function declarations
void initWebServer();
void handleWebSocket();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void handleRoot();
void handleAPI();
void handleGetStatus();
void handleSetRelay();
void handleGetPeers();
void handleSendCommand();
void handlePairing();
void handleNotFound();
void handleStyle();
void handleScript();

// Utility functions
String getStatusJSON();
String getPeersJSON();
String generateWebPage();

// WebSocket server reference
extern WebSocketsServer webSocket;

#endif // WEB_INTERFACE_H