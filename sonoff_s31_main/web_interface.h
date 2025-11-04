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

// External relay control functions (defined in main .ino file)
extern void turnOnRelay();
extern void turnOffRelay();
extern void toggleRelay();
void handleGetPeers();
void handleSendCommand();
void handlePairing();
void handleWiFiConfig();
void handleSetWiFiConfig();
void handleNotFound();
void handleStyle();
void handleStatusJS();
void handlePeersJS();
void handleRelayJS();
void handleWiFiJS();
void handlePairingJS();

// Utility functions
String getStatusJSON();
String getPeersJSON();
String generateWebPage();

// WiFi configuration functions
void loadWiFiConfig();
void saveWiFiConfig();
void clearWiFiConfig();

// Global WiFi configuration
extern WiFiConfig wifiConfig;

#endif // WEB_INTERFACE_H