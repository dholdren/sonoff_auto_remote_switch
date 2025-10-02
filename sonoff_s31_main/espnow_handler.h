/*
 * ESP-NOW Communication Handler
 * For SONOFF S31 ESP8266 Project
 */

#ifndef ESPNOW_HANDLER_H
#define ESPNOW_HANDLER_H

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ArduinoJson.h>

// ESP-NOW message types
enum ESPNOWMessageType {
  MSG_DEVICE_STATE = 1,
  MSG_COMMAND = 2,
  MSG_DISCOVERY = 3,
  MSG_HEARTBEAT = 4,
  MSG_PAIRING = 5,
  MSG_PAIRING_RESPONSE = 6,
  MSG_CURRENT_HIGH = 7,
  MSG_CURRENT_LOW = 8
};

// ESP-NOW message structure
struct ESPNOWMessage {
  uint8_t messageType;
  uint8_t deviceId[6];
  uint32_t timestamp;
  char payload[200];
};

// Peer device structure
struct ESPNOWPeer {
  uint8_t mac[6];
  String deviceId;
  unsigned long lastSeen;
  bool isOnline;
};

// Function declarations
void initESPNOW();
void handleESPNOWMessages();
void broadcastDeviceState();
void broadcastHeartbeat();
void sendCommand(uint8_t* targetMac, const String& command, const String& value);
void onESPNOWDataReceived(uint8_t *mac, uint8_t *data, uint8_t len);
void onESPNOWDataSent(uint8_t *mac, uint8_t status);
void addPeer(uint8_t* mac);
void removePeer(uint8_t* mac);
void updatePeerList();
String macToString(uint8_t* mac);
void stringToMac(const String& macStr, uint8_t* mac);

// Pairing functions
void enterPairingMode();
void exitPairingMode();
void handlePairingMode();
void sendPairingMessage(bool isParent);
void processPairingMessage(uint8_t* senderMac, const char* payload);
void savePairingData();
void loadPairingData();
bool addChild(uint8_t* childMac);
bool setParent(uint8_t* parentMac);
uint32_t calculateChecksum(const PairingData* data);
void clearPairingData();
void printPairingStatus();

// Current automation functions
void sendCurrentAlert(bool isHigh);
void handleCurrentAlert(uint8_t* senderMac, bool isHigh);

// Relay control functions (defined in main .ino file)
void turnOnRelay();
void turnOffRelay();

// Global variables
extern ESPNOWPeer espnowPeers[MAX_ESPNOW_PEERS];
extern int espnowPeerCount;
extern CurrentAutomation currentAutomation;

#endif // ESPNOW_HANDLER_H