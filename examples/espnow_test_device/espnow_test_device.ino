/*
 * Example ESP-NOW Receiver/Sender for testing
 * This can be used on another ESP8266 device to test ESP-NOW communication
 */

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ArduinoJson.h>

// Message structure (must match main device)
struct ESPNOWMessage {
  uint8_t messageType;
  uint8_t deviceId[6];
  uint32_t timestamp;
  char payload[200];
};

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- ESP-NOW Test Device ---");
  
  WiFi.mode(WIFI_STA);
  
  // Initialize ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(onDataReceived);
  esp_now_register_send_cb(onDataSent);
  
  // Add broadcast peer
  uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_add_peer(broadcastMac, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  
  Serial.println("ESP-NOW Test Device Ready");
  Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
}

void loop() {
  // Send test command every 10 seconds
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 10000) {
    sendTestCommand();
    lastSend = millis();
  }
  
  delay(100);
}

void onDataReceived(uint8_t *mac, uint8_t *data, uint8_t len) {
  if (len != sizeof(ESPNOWMessage)) {
    return;
  }
  
  ESPNOWMessage* msg = (ESPNOWMessage*)data;
  
  Serial.printf("Received message type %d from %02X:%02X:%02X:%02X:%02X:%02X\n", 
                msg->messageType, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.printf("Payload: %s\n", msg->payload);
  
  // Parse JSON payload if it's a device state message
  if (msg->messageType == 1) { // MSG_DEVICE_STATE
    DynamicJsonDocument doc(200);
    deserializeJson(doc, msg->payload);
    
    Serial.printf("Device: %s, Relay: %s, Power: %.2fW\n",
                  doc["deviceId"].as<String>().c_str(),
                  doc["relay"].as<bool>() ? "ON" : "OFF",
                  doc["power"].as<float>());
  }
}

void onDataSent(uint8_t *mac, uint8_t status) {
  Serial.printf("Send status to %02X:%02X:%02X:%02X:%02X:%02X: %s\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                status == 0 ? "Success" : "Failed");
}

void sendTestCommand() {
  ESPNOWMessage msg;
  msg.messageType = 2; // MSG_COMMAND
  msg.timestamp = millis();
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  memcpy(msg.deviceId, mac, 6);
  
  // Create command to toggle relay
  DynamicJsonDocument doc(200);
  doc["command"] = "relay";
  doc["value"] = "toggle";
  doc["sender"] = "ESP_TEST_" + String(ESP.getChipId(), HEX);
  
  serializeJson(doc, msg.payload, sizeof(msg.payload));
  
  uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_send(broadcastMac, (uint8_t*)&msg, sizeof(msg));
  
  Serial.println("Test command sent: toggle relay");
}