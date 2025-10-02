/*
 * ESP-NOW Communication Handler Implementation
 * For SONOFF S31 ESP8266 Project
 */

#include "config.h"
#include "espnow_handler.h"
#include <LittleFS.h>
#include "Logger.h"

// Global variables
ESPNOWPeer espnowPeers[MAX_ESPNOW_PEERS];
int espnowPeerCount = 0;
extern DeviceState deviceState;

void initESPNOW() {
  // Set device in AP+STA mode for ESP-NOW
  WiFi.mode(WIFI_AP_STA);
  
  // Initialize ESP-NOW
  if (esp_now_init() != 0) {
    logger.println("Error initializing ESP-NOW");
    return;
  }
  
  // Set ESP-NOW role
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  
  // Register callbacks
  esp_now_register_recv_cb(onESPNOWDataReceived);
  esp_now_register_send_cb(onESPNOWDataSent);
  
  // Add broadcast peer for discovery
  uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_add_peer(broadcastMac, ESP_NOW_ROLE_COMBO, ESPNOW_CHANNEL, NULL, 0);
  
  logger.println("ESP-NOW initialized successfully");
  
  #if DEBUG_ESPNOW
  logger.printf("ESP-NOW MAC: %s\n", WiFi.macAddress().c_str());
  #endif
}

void handleESPNOWMessages() {
  // Update peer list (remove offline peers)
  updatePeerList();
  
  // Send periodic heartbeat
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 30000) { // Every 30 seconds
    broadcastHeartbeat();
    lastHeartbeat = millis();
  }
}

void broadcastDeviceState() {
  ESPNOWMessage msg;
  msg.messageType = MSG_DEVICE_STATE;
  msg.timestamp = millis();
  
  // Copy device MAC to message
  uint8_t mac[6];
  WiFi.macAddress(mac);
  memcpy(msg.deviceId, mac, 6);
  
  // Create JSON payload
  DynamicJsonDocument doc(200);
  doc["deviceId"] = deviceState.deviceId;
  doc["relay"] = deviceState.relayState;
  doc["voltage"] = deviceState.voltage;
  doc["current"] = deviceState.current;
  doc["power"] = deviceState.power;
  doc["energy"] = deviceState.energy;
  doc["wifi"] = deviceState.wifiConnected;
  doc["uptime"] = millis();
  
  serializeJson(doc, msg.payload, sizeof(msg.payload));
  
  // Broadcast to all peers
  uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_send(broadcastMac, (uint8_t*)&msg, sizeof(msg));
  
  #if DEBUG_ESPNOW
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 10000) {
    logger.println("ESP-NOW: Device state broadcasted");
    lastDebug = millis();
  }
  #endif
}

void broadcastHeartbeat() {
  ESPNOWMessage msg;
  msg.messageType = MSG_HEARTBEAT;
  msg.timestamp = millis();
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  memcpy(msg.deviceId, mac, 6);
  
  strcpy(msg.payload, deviceState.deviceId.c_str());
  
  uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_send(broadcastMac, (uint8_t*)&msg, sizeof(msg));
}

void sendCommand(uint8_t* targetMac, const String& command, const String& value) {
  ESPNOWMessage msg;
  msg.messageType = MSG_COMMAND;
  msg.timestamp = millis();
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  memcpy(msg.deviceId, mac, 6);
  
  DynamicJsonDocument doc(200);
  doc["command"] = command;
  doc["value"] = value;
  doc["sender"] = deviceState.deviceId;
  
  serializeJson(doc, msg.payload, sizeof(msg.payload));
  
  esp_now_send(targetMac, (uint8_t*)&msg, sizeof(msg));
  
  #if DEBUG_ESPNOW
  logger.printf("ESP-NOW: Command sent to %s: %s=%s\n", 
                macToString(targetMac).c_str(), command.c_str(), value.c_str());
  #endif
}

void onESPNOWDataReceived(uint8_t *mac, uint8_t *data, uint8_t len) {
  if (len != sizeof(ESPNOWMessage)) {
    return;
  }
  
  ESPNOWMessage* msg = (ESPNOWMessage*)data;
  
  #if DEBUG_ESPNOW
  logger.printf("ESP-NOW: Received message type %d from %s\n", 
                msg->messageType, macToString(mac).c_str());
  #endif
  
  // Add or update peer
  addPeer(mac);
  
  switch (msg->messageType) {
    case MSG_DEVICE_STATE: {
      // Process device state update
      DynamicJsonDocument doc(200);
      deserializeJson(doc, msg->payload);
      
      // Update peer information
      for (int i = 0; i < espnowPeerCount; i++) {
        if (memcmp(espnowPeers[i].mac, mac, 6) == 0) {
          espnowPeers[i].deviceId = doc["deviceId"].as<String>();
          espnowPeers[i].lastSeen = millis();
          espnowPeers[i].isOnline = true;
          break;
        }
      }
      break;
    }
    
    case MSG_COMMAND: {
      // Process incoming command
      DynamicJsonDocument doc(200);
      deserializeJson(doc, msg->payload);
      
      String command = doc["command"];
      String value = doc["value"];
      
      if (command == "relay") {
        if (value == "on" || value == "1") {
          deviceState.relayState = true;
          digitalWrite(RELAY_PIN, HIGH);
        } else if (value == "off" || value == "0") {
          deviceState.relayState = false;
          digitalWrite(RELAY_PIN, LOW);
        } else if (value == "toggle") {
          deviceState.relayState = !deviceState.relayState;
          digitalWrite(RELAY_PIN, deviceState.relayState ? HIGH : LOW);
        }
        
        logger.printf("ESP-NOW: Relay command received: %s\n", value.c_str());
        broadcastDeviceState(); // Broadcast updated state
      }
      break;
    }
    
    case MSG_DISCOVERY: {
      // Respond to discovery request
      broadcastDeviceState();
      break;
    }
    
    case MSG_HEARTBEAT: {
      // Update peer last seen time
      for (int i = 0; i < espnowPeerCount; i++) {
        if (memcmp(espnowPeers[i].mac, mac, 6) == 0) {
          espnowPeers[i].lastSeen = millis();
          espnowPeers[i].isOnline = true;
          break;
        }
      }
      break;
    }
    
    case MSG_PAIRING:
    case MSG_PAIRING_RESPONSE: {
      // Process pairing message
      processPairingMessage(mac, msg->payload);
      break;
    }
    
    case MSG_CURRENT_HIGH:
    case MSG_CURRENT_LOW: {
      // Process current alert message
      bool isHigh = (msg->messageType == MSG_CURRENT_HIGH);
      handleCurrentAlert(mac, isHigh);
      break;
    }
  }
}

void onESPNOWDataSent(uint8_t *mac, uint8_t status) {
  #if DEBUG_ESPNOW
  if (status != 0) {
    logger.printf("ESP-NOW: Send failed to %s, status: %d\n", 
                  macToString(mac).c_str(), status);
  }
  #endif
}

void addPeer(uint8_t* mac) {
  // Check if peer already exists
  for (int i = 0; i < espnowPeerCount; i++) {
    if (memcmp(espnowPeers[i].mac, mac, 6) == 0) {
      espnowPeers[i].lastSeen = millis();
      espnowPeers[i].isOnline = true;
      return;
    }
  }
  
  // Add new peer if there's space
  if (espnowPeerCount < MAX_ESPNOW_PEERS) {
    memcpy(espnowPeers[espnowPeerCount].mac, mac, 6);
    espnowPeers[espnowPeerCount].lastSeen = millis();
    espnowPeers[espnowPeerCount].isOnline = true;
    espnowPeerCount++;
    
    // Add to ESP-NOW peer list
    esp_now_add_peer(mac, ESP_NOW_ROLE_COMBO, ESPNOW_CHANNEL, NULL, 0);
    
    #if DEBUG_ESPNOW
    logger.printf("ESP-NOW: New peer added: %s\n", macToString(mac).c_str());
    #endif
  }
}

void removePeer(uint8_t* mac) {
  for (int i = 0; i < espnowPeerCount; i++) {
    if (memcmp(espnowPeers[i].mac, mac, 6) == 0) {
      // Remove from ESP-NOW peer list
      esp_now_del_peer(mac);
      
      // Shift array elements
      for (int j = i; j < espnowPeerCount - 1; j++) {
        espnowPeers[j] = espnowPeers[j + 1];
      }
      espnowPeerCount--;
      
      #if DEBUG_ESPNOW
      logger.printf("ESP-NOW: Peer removed: %s\n", macToString(mac).c_str());
      #endif
      break;
    }
  }
}

void updatePeerList() {
  unsigned long currentTime = millis();
  
  for (int i = 0; i < espnowPeerCount; i++) {
    if (currentTime - espnowPeers[i].lastSeen > 60000) { // 1 minute timeout
      espnowPeers[i].isOnline = false;
      
      // Remove completely offline peers after 5 minutes
      if (currentTime - espnowPeers[i].lastSeen > 300000) {
        removePeer(espnowPeers[i].mac);
        i--; // Adjust index after removal
      }
    }
  }
}

String macToString(uint8_t* mac) {
  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

void stringToMac(const String& macStr, uint8_t* mac) {
  sscanf(macStr.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X", 
         &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}

// ===== PAIRING SYSTEM IMPLEMENTATION =====

void enterPairingMode() {
  if (deviceState.pairingMode) {
    return; // Already in pairing mode
  }
  
  deviceState.pairingMode = true;
  deviceState.pairingStartTime = millis();
  
  logger.println("\n=== ENTERING PAIRING MODE ===");
  logger.println("Listening for parent devices for 5 seconds...");
  
  // Wait 5 seconds to listen for existing parent devices
  unsigned long listenStart = millis();
  bool parentFound = false;
  
  while (millis() - listenStart < 5000) {
    // Process any incoming ESP-NOW messages
    delay(100);
    
    // Check if we received a pairing message with parent flag
    // This would be handled in processPairingMessage function
  }
  
  // If no parent found after 5 seconds, become parent
  if (!deviceState.hasParent) {
    deviceState.isParent = true;
    logger.println("No parent found - becoming PARENT device");
    logger.println("LED will blink slowly, sending pairing messages...");
  } else {
    logger.println("Parent device found - remaining as CHILD device");
    logger.println("LED will blink fast, listening for parent confirmation...");
  }
  
  printPairingStatus();
}

void exitPairingMode() {
  if (!deviceState.pairingMode) {
    return; // Not in pairing mode
  }
  
  deviceState.pairingMode = false;
  
  logger.println("\n=== EXITING PAIRING MODE ===");
  
  // Save pairing data to flash
  savePairingData();
  
  logger.println("Pairing data saved to flash storage");
  printPairingStatus();
}

void handlePairingMode() {
  if (!deviceState.pairingMode) {
    return;
  }
  
  unsigned long currentTime = millis();
  
  // Check for pairing timeout
  if (currentTime - deviceState.pairingStartTime > PAIRING_MODE_TIMEOUT) {
    logger.println("Pairing mode timeout - exiting");
    exitPairingMode();
    return;
  }
  
  // Send pairing messages if we're a parent
  static unsigned long lastPairingBroadcast = 0;
  if (deviceState.isParent && currentTime - lastPairingBroadcast > 2000) {
    sendPairingMessage(true); // Send as parent
    lastPairingBroadcast = currentTime;
  }
  
  // Send pairing request if we're looking for a parent
  if (!deviceState.hasParent && !deviceState.isParent) {
    static unsigned long lastPairingRequest = 0;
    if (currentTime - lastPairingRequest > 3000) {
      sendPairingMessage(false); // Send as child looking for parent
      lastPairingRequest = currentTime;
    }
  }
}

void sendPairingMessage(bool isParent) {
  ESPNOWMessage msg;
  msg.messageType = MSG_PAIRING;
  msg.timestamp = millis();
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  memcpy(msg.deviceId, mac, 6);
  
  DynamicJsonDocument doc(200);
  doc["deviceId"] = deviceState.deviceId;
  doc["isParent"] = isParent;
  doc["hasParent"] = deviceState.hasParent;
  doc["childCount"] = deviceState.childCount;
  
  if (deviceState.hasParent) {
    doc["parentMac"] = macToString(deviceState.parentMac);
  }
  
  serializeJson(doc, msg.payload, sizeof(msg.payload));
  
  uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_send(broadcastMac, (uint8_t*)&msg, sizeof(msg));
  
  #if DEBUG_ESPNOW
  logger.printf("Pairing message sent - isParent: %s\n", isParent ? "true" : "false");
  #endif
}

void processPairingMessage(uint8_t* senderMac, const char* payload) {
  DynamicJsonDocument doc(200);
  deserializeJson(doc, payload);
  
  String senderDeviceId = doc["deviceId"];
  bool senderIsParent = doc["isParent"];
  bool senderHasParent = doc["hasParent"];
  
  #if DEBUG_ESPNOW
  logger.printf("Pairing message from %s: isParent=%s, hasParent=%s\n", 
                macToString(senderMac).c_str(), 
                senderIsParent ? "true" : "false",
                senderHasParent ? "true" : "false");
  #endif
  
  // If we're in pairing mode and sender is a parent, set them as our parent
  if (deviceState.pairingMode && !deviceState.hasParent && senderIsParent) {
    if (setParent(senderMac)) {
      logger.printf("Set parent device: %s (%s)\n", 
                   macToString(senderMac).c_str(), senderDeviceId.c_str());
      
      // Send pairing response
      ESPNOWMessage response;
      response.messageType = MSG_PAIRING_RESPONSE;
      response.timestamp = millis();
      
      uint8_t mac[6];
      WiFi.macAddress(mac);
      memcpy(response.deviceId, mac, 6);
      
      DynamicJsonDocument responseDoc(200);
      responseDoc["deviceId"] = deviceState.deviceId;
      responseDoc["accepted"] = true;
      serializeJson(responseDoc, response.payload, sizeof(response.payload));
      
      esp_now_send(senderMac, (uint8_t*)&response, sizeof(response));
    }
  }
  
  // If we're a parent and sender wants to pair, add them as child
  if (deviceState.isParent && deviceState.pairingMode && !senderHasParent) {
    if (addChild(senderMac)) {
      logger.printf("Added child device: %s (%s)\n", 
                   macToString(senderMac).c_str(), senderDeviceId.c_str());
    }
  }
}

bool setParent(uint8_t* parentMac) {
  if (deviceState.hasParent) {
    logger.println("Already have a parent - ignoring");
    return false;
  }
  
  memcpy(deviceState.parentMac, parentMac, 6);
  deviceState.hasParent = true;
  deviceState.isParent = false;
  
  // Add parent to ESP-NOW peer list
  esp_now_add_peer(parentMac, ESP_NOW_ROLE_COMBO, ESPNOW_CHANNEL, NULL, 0);
  
  return true;
}

bool addChild(uint8_t* childMac) {
  if (deviceState.childCount >= MAX_CHILDREN) {
    logger.println("Maximum children reached - cannot add more");
    return false;
  }
  
  // Check if already a child
  for (int i = 0; i < deviceState.childCount; i++) {
    if (memcmp(deviceState.childMacs[i], childMac, 6) == 0) {
      logger.println("Device already registered as child");
      return false;
    }
  }
  
  memcpy(deviceState.childMacs[deviceState.childCount], childMac, 6);
  deviceState.childCount++;
  
  // Add child to ESP-NOW peer list
  esp_now_add_peer(childMac, ESP_NOW_ROLE_COMBO, ESPNOW_CHANNEL, NULL, 0);
  
  return true;
}

void savePairingData() {
  PairingData data;
  
  // Initialize structure
  memset(&data, 0, sizeof(PairingData));
  
  data.magic = FLASH_MAGIC;
  data.version = FLASH_VERSION;
  data.isParent = deviceState.isParent;
  data.hasParent = deviceState.hasParent;
  data.childCount = deviceState.childCount;
  
  if (deviceState.hasParent) {
    memcpy(data.parentMac, deviceState.parentMac, 6);
  }
  
  for (int i = 0; i < deviceState.childCount && i < MAX_CHILDREN; i++) {
    memcpy(data.childMacs[i], deviceState.childMacs[i], 6);
  }
  
  data.checksum = calculateChecksum(&data);
  
  // Write to LittleFS
  File file = LittleFS.open(PAIRING_FILE, "w");
  if (file) {
    file.write((uint8_t*)&data, sizeof(PairingData));
    file.close();
    logger.println("Pairing data saved to flash storage");
  } else {
    logger.println("Failed to open pairing file for writing");
  }
}

void loadPairingData() {
  PairingData data;
  
  // Check if pairing file exists
  if (!LittleFS.exists(PAIRING_FILE)) {
    logger.println("No pairing data file found - using defaults");
    clearPairingData();
    return;
  }
  
  // Read from LittleFS
  File file = LittleFS.open(PAIRING_FILE, "r");
  if (!file) {
    logger.println("Failed to open pairing file for reading - using defaults");
    clearPairingData();
    return;
  }
  
  if (file.size() != sizeof(PairingData)) {
    logger.println("Pairing file size mismatch - using defaults");
    file.close();
    clearPairingData();
    return;
  }
  
  file.read((uint8_t*)&data, sizeof(PairingData));
  file.close();
  
  // Validate magic number and version
  if (data.magic != FLASH_MAGIC || data.version != FLASH_VERSION) {
    logger.println("No valid pairing data found in flash - using defaults");
    clearPairingData();
    return;
  }
  
  // Verify checksum
  uint32_t storedChecksum = data.checksum;
  data.checksum = 0; // Clear for calculation
  uint32_t calculatedChecksum = calculateChecksum(&data);
  
  if (storedChecksum != calculatedChecksum) {
    logger.println("Flash data checksum mismatch - using defaults");
    clearPairingData();
    return;
  }
  
  // Load valid data
  deviceState.isParent = data.isParent;
  deviceState.hasParent = data.hasParent;
  deviceState.childCount = data.childCount;
  
  if (deviceState.hasParent) {
    memcpy(deviceState.parentMac, data.parentMac, 6);
    // Add parent to ESP-NOW peer list
    esp_now_add_peer(deviceState.parentMac, ESP_NOW_ROLE_COMBO, ESPNOW_CHANNEL, NULL, 0);
  }
  
  for (int i = 0; i < deviceState.childCount && i < MAX_CHILDREN; i++) {
    memcpy(deviceState.childMacs[i], data.childMacs[i], 6);
    // Add child to ESP-NOW peer list
    esp_now_add_peer(deviceState.childMacs[i], ESP_NOW_ROLE_COMBO, ESPNOW_CHANNEL, NULL, 0);
  }
  
  logger.println("Pairing data loaded from flash storage");
  printPairingStatus();
}

uint32_t calculateChecksum(const PairingData* data) {
  uint32_t checksum = 0;
  const uint8_t* bytes = (const uint8_t*)data;
  size_t size = sizeof(PairingData) - sizeof(uint32_t); // Exclude checksum field
  
  for (size_t i = 0; i < size; i++) {
    checksum += bytes[i];
  }
  
  return checksum;
}

void clearPairingData() {
  deviceState.isParent = false;
  deviceState.hasParent = false;
  deviceState.childCount = 0;
  memset(deviceState.parentMac, 0, 6);
  memset(deviceState.childMacs, 0, sizeof(deviceState.childMacs));
  
  // Remove pairing file from flash storage
  if (LittleFS.exists(PAIRING_FILE)) {
    LittleFS.remove(PAIRING_FILE);
    logger.println("Pairing file removed from flash storage");
  }
  
  logger.println("Pairing data cleared");
}

void printPairingStatus() {
  logger.println("\n=== PAIRING STATUS ===");
  logger.printf("Device ID: %s\n", deviceState.deviceId.c_str());
  logger.printf("Is Parent: %s\n", deviceState.isParent ? "YES" : "NO");
  logger.printf("Has Parent: %s\n", deviceState.hasParent ? "YES" : "NO");
  
  if (deviceState.hasParent) {
    logger.printf("Parent MAC: %s\n", macToString(deviceState.parentMac).c_str());
  }
  
  logger.printf("Children: %d/%d\n", deviceState.childCount, MAX_CHILDREN);
  for (int i = 0; i < deviceState.childCount; i++) {
    logger.printf("  Child %d: %s\n", i + 1, macToString(deviceState.childMacs[i]).c_str());
  }
  
  logger.printf("Pairing Mode: %s\n", deviceState.pairingMode ? "ACTIVE" : "INACTIVE");
  logger.println("=====================\n");
}

void sendCurrentAlert(bool isHigh) {
  if (!deviceState.isParent || deviceState.childCount == 0) {
    return; // Only parents with children should send alerts
  }
  
  ESPNOWMessage msg;
  msg.messageType = isHigh ? MSG_CURRENT_HIGH : MSG_CURRENT_LOW;
  msg.payload[0] = isHigh ? 1 : 0;  // Simple payload indicating high/lowq

  // Send alert to all children
  for (int i = 0; i < deviceState.childCount; i++) {
    esp_now_send(deviceState.childMacs[i], (uint8_t*)&msg, sizeof(ESPNOWMessage));
    #if DEBUG_ESPNOW
    logger.printf("ESP-NOW: Sent current %s alert to child %s\n", 
                  isHigh ? "HIGH" : "LOW", 
                  macToString(deviceState.childMacs[i]).c_str());
    #endif
  }
}

void handleCurrentAlert(uint8_t* senderMac, bool isHigh) {
  // Only children should respond to current alerts
  if (!deviceState.hasParent) {
    return;
  }
  
  // Verify the alert is from our parent
  bool isFromParent = true;
  for (int i = 0; i < 6; i++) {
    if (senderMac[i] != deviceState.parentMac[i]) {
      isFromParent = false;
      break;
    }
  }
  
  if (!isFromParent) {
    #if DEBUG_ESPNOW
    logger.printf("ESP-NOW: Ignoring current alert from non-parent %s\n", 
                  macToString(senderMac).c_str());
    #endif
    return;
  }
  
  #if DEBUG_ESPNOW
  logger.printf("ESP-NOW: Received current %s alert from parent\n", isHigh ? "HIGH" : "LOW");
  #endif
  
  if (isHigh) {
    // Turn on immediately when parent current goes high
    turnOnRelay();
    currentAutomation.childTurnOffTimer = 0; // Cancel any pending turn-off
    logger.println("Child: Turning ON due to parent high current");
  } else {
    // Turn off after 3 seconds when parent current goes low
    currentAutomation.childTurnOffTimer = millis() + CHILD_TURN_OFF_DELAY;
    logger.println("Child: Scheduled turn OFF in 3 seconds due to parent low current");
  }
}