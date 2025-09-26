/*
 * ESP-NOW Communication Handler Implementation
 * For SONOFF S31 ESP8266 Project
 */

#include "espnow_handler.h"
#include "config.h"

// Global variables
ESPNOWPeer espnowPeers[MAX_ESPNOW_PEERS];
int espnowPeerCount = 0;
extern struct DeviceState deviceState;

void initESPNOW() {
  // Set device in AP+STA mode for ESP-NOW
  WiFi.mode(WIFI_AP_STA);
  
  // Initialize ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
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
  
  Serial.println("ESP-NOW initialized successfully");
  
  #if DEBUG_ESPNOW
  Serial.printf("ESP-NOW MAC: %s\n", WiFi.macAddress().c_str());
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
    Serial.println("ESP-NOW: Device state broadcasted");
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
  Serial.printf("ESP-NOW: Command sent to %s: %s=%s\n", 
                macToString(targetMac).c_str(), command.c_str(), value.c_str());
  #endif
}

void onESPNOWDataReceived(uint8_t *mac, uint8_t *data, uint8_t len) {
  if (len != sizeof(ESPNOWMessage)) {
    return;
  }
  
  ESPNOWMessage* msg = (ESPNOWMessage*)data;
  
  #if DEBUG_ESPNOW
  Serial.printf("ESP-NOW: Received message type %d from %s\n", 
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
        
        Serial.printf("ESP-NOW: Relay command received: %s\n", value.c_str());
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
  }
}

void onESPNOWDataSent(uint8_t *mac, uint8_t status) {
  #if DEBUG_ESPNOW
  if (status != 0) {
    Serial.printf("ESP-NOW: Send failed to %s, status: %d\n", 
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
    Serial.printf("ESP-NOW: New peer added: %s\n", macToString(mac).c_str());
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
      Serial.printf("ESP-NOW: Peer removed: %s\n", macToString(mac).c_str());
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