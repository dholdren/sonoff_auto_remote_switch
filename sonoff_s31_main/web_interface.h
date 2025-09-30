/*
 * Web Interface Handler
 * For SONOFF S31 ESP8266 Project
 */

#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

// Function declarations
void initWebServer();
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

#endif // WEB_INTERFACE_H