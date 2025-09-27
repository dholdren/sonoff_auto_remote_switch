/*
 * SONOFF S31 ESP8266 Project with ESP-NOW and CSE7766 Sensor
 * Features:
 * - ESP-NOW communication for device mesh network
 * - WiFi web dashboard for monitoring and control
 * - CSE7766 power monitoring sensor integration
 * - Relay control for power switching
 * - LED status indicators
 */

#include "config.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <espnow.h>
#include <SoftwareSerial.h>
#include "CSE7766.h"
#include "web_interface.h"
#include "espnow_handler.h"

// Global objects
ESP8266WebServer server(80);
SoftwareSerial cseSerial(CSE7766_RX_PIN, CSE7766_TX_PIN);
CSE7766 cse7766(cseSerial);

// Device state
DeviceState deviceState;

// Button handling
unsigned long buttonPressTime = 0;
bool buttonPressed = false;

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- SONOFF S31 ESP8266 Starting ---");
  
  // Initialize device ID
  deviceState.deviceId = "SONOFF_S31_" + String(ESP.getChipId(), HEX);
  
  // Initialize hardware pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initial state
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, HIGH); // LED off (inverted)
  
  // Initialize CSE7766 sensor
  cseSerial.begin(4800);
  cse7766.begin();
  Serial.println("CSE7766 sensor initialized");
  
  // Initialize WiFi
  initWiFi();
  
  // Initialize ESP-NOW
  initESPNOW();
  
  // Initialize web server
  initWebServer();
  
  // Start mDNS
  if (MDNS.begin("sonoff-s31")) {
    Serial.println("mDNS responder started: sonoff-s31.local");
  }
  
  Serial.printf("Device ID: %s\n", deviceState.deviceId.c_str());
  Serial.println("Setup completed successfully!");
}

void loop() {
  // Handle web server
  server.handleClient();
  MDNS.update();
  
  // Handle button press
  handleButton();
  
  // Update sensor readings
  updateSensorReadings();
  
  // Handle ESP-NOW messages
  handleESPNOWMessages();
  
  // Update LED status
  updateLEDStatus();
  
  // Send periodic ESP-NOW broadcasts
  static unsigned long lastBroadcast = 0;
  if (millis() - lastBroadcast > ESPNOW_BROADCAST_INTERVAL) {
    broadcastDeviceState();
    lastBroadcast = millis();
  }
  
  delay(100);
}

void initWiFi() {
  WiFi.mode(WIFI_AP_STA);
  
  // Start Access Point
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.printf("Access Point started: %s\n", AP_SSID);
  Serial.printf("AP IP address: %s\n", WiFi.softAPIP().toString().c_str());
  
  // Connect to WiFi network
  if (strlen(WIFI_SSID) > 0) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("Connecting to WiFi: %s", WIFI_SSID);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      deviceState.wifiConnected = true;
      Serial.printf("\nWiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
      Serial.println("\nWiFi connection failed, continuing with AP mode only");
    }
  }
}

void handleButton() {
  bool currentButtonState = !digitalRead(BUTTON_PIN); // Inverted logic
  
  if (currentButtonState && !buttonPressed) {
    buttonPressed = true;
    buttonPressTime = millis();
  } else if (!currentButtonState && buttonPressed) {
    buttonPressed = false;
    unsigned long pressDuration = millis() - buttonPressTime;
    
    if (pressDuration > 50 && pressDuration < 3000) {
      // Short press: toggle relay
      toggleRelay();
    } else if (pressDuration >= 3000) {
      // Long press: reset WiFi settings (for future implementation)
      Serial.println("Long press detected - WiFi reset requested");
    }
  }
}

void toggleRelay() {
  deviceState.relayState = !deviceState.relayState;
  digitalWrite(RELAY_PIN, deviceState.relayState ? HIGH : LOW);
  Serial.printf("Relay %s\n", deviceState.relayState ? "ON" : "OFF");
  
  // Broadcast state change via ESP-NOW
  broadcastDeviceState();
}

void updateSensorReadings() {
  static unsigned long lastReading = 0;
  if (millis() - lastReading > 1000) { // Update every second
    if (cse7766.available()) {
      deviceState.voltage = cse7766.getVoltage();
      deviceState.current = cse7766.getCurrent();
      deviceState.power = cse7766.getActivePower();
      deviceState.energy += deviceState.power / 3600.0; // Convert to Wh
      deviceState.lastUpdate = millis();
      
      // Debug output every 10 seconds
      static unsigned long lastDebug = 0;
      if (millis() - lastDebug > 10000) {
        Serial.printf("Power: %.2fW, Voltage: %.1fV, Current: %.3fA, Energy: %.2fWh\n",
                     deviceState.power, deviceState.voltage, deviceState.current, deviceState.energy);
        lastDebug = millis();
      }
    }
    lastReading = millis();
  }
}

void updateLEDStatus() {
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  
  if (deviceState.wifiConnected) {
    // Solid on when WiFi connected
    digitalWrite(LED_PIN, LOW);
  } else {
    // Blink when no WiFi
    if (millis() - lastBlink > 500) {
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? LOW : HIGH);
      lastBlink = millis();
    }
  }
}