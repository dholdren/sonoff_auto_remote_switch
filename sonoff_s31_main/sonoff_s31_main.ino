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
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include "CSE7766.h"
#include "web_interface.h"
#include "espnow_handler.h"
#include "Logger.h"

// Global objects
ESP8266WebServer server(80);
CSE7766 cse7766; //Uses Serial, the sensor is connected to Serial RX

// Device state
DeviceState deviceState;

// Button handling
unsigned long buttonPressTime = 0;
bool buttonPressed = false;
String UNIQUE_ID = String(ESP.getChipId(), HEX);
String HOSTNAME = "sonoff-s31-" + UNIQUE_ID;

// Current-based automation
CurrentAutomation currentAutomation;
bool childPendingTurnOff = false;       // Flag for pending child turn-off

// Function declarations
void saveRelayState();
void loadRelayState();

void setup() {
  // Initialize CSE7766 sensor
  cse7766.begin(); //will call Serial.begin()
  
  // Initialize device ID
  deviceState.deviceId = "SONOFF_S31_" + UNIQUE_ID;

  // Initialize hardware pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initial state
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, HIGH);  // LED off (inverted)

  // Initialize LittleFS for pairing data storage
  if (!LittleFS.begin()) {
    logger.println("Failed to mount LittleFS filesystem");
  } else {
    logger.println("LittleFS filesystem mounted successfully");
  }
  
  // Initialize WiFi
  initWiFi();
  
  // Load pairing data from flash
  loadPairingData();
  
  // Initialize ESP-NOW
  initESPNOW();
  
  // Load saved relay state from flash (after ESP-NOW is initialized)
  loadRelayState();
  
  // Initialize current automation variables
  currentAutomation.lastCurrentState = false;
  currentAutomation.childTurnOffTimer = 0;
  
  // Initialize web server
  initWebServer();
  
  // Initialize OTA updates
  initOTA();
  
  // Start mDNS
  if (MDNS.begin(HOSTNAME)) {
    Serial.printf("mDNS responder started: %s\n", HOSTNAME.c_str());
  }
  
  // Print device information
  logger.printf("Device ID: %s\n", deviceState.deviceId.c_str());
  logger.println("Setup completed successfully!");
}

void loop() {
  // Log memory usage periodically
  static unsigned long lastMemoryLog = 0;
  if (millis() - lastMemoryLog > 10000) { // Every 10 seconds
    uint32_t freeHeap = ESP.getFreeHeap();
    uint8_t fragmentation = ESP.getHeapFragmentation();
    logger.printf("Memory: Free=%u bytes, Fragmentation=%u%%\n", freeHeap, fragmentation);
    lastMemoryLog = millis();
  }
  
  // Handle OTA updates
  ArduinoOTA.handle();
  
  // Handle web server
  server.handleClient();
  MDNS.update();
  
  // Handle WebSocket connections
  handleWebSocket();
  
  // Handle button press
  handleButton();
  
  // Update sensor readings
  updateSensorReadings();
  
  // Handle ESP-NOW messages
  handleESPNOWMessages();
  
  // Handle current automation timer for children
  if (currentAutomation.childTurnOffTimer > 0 && millis() >= currentAutomation.childTurnOffTimer) {
    turnOffRelay();
    currentAutomation.childTurnOffTimer = 0;
    logger.println("Child: Turning OFF after 3-second delay");
  }
  
  // Handle pairing mode
  handlePairingMode();
  
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
  String UNIQUE_AP_SSID = String(AP_SSID) + String("-") + UNIQUE_ID;
  WiFi.softAP(UNIQUE_AP_SSID, AP_PASSWORD);
  logger.printf("Access Point started: %s\n", WiFi.softAPSSID().c_str());

  logger.printf("AP IP address: %s\n", WiFi.softAPIP().toString().c_str());
  
  // Load WiFi configuration from flash
  loadWiFiConfig();
  
  // Connect to WiFi network if configured
  if (wifiConfig.isConfigured && strlen(wifiConfig.ssid) > 0) {
    WiFi.begin(wifiConfig.ssid, wifiConfig.password);
    logger.printf("Connecting to WiFi: %s", wifiConfig.ssid);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      logger.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      deviceState.wifiConnected = true;
      logger.printf("\nWiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
      logger.println("\nWiFi connection failed, continuing with AP mode only");
    }
  } else {
    logger.println("No WiFi configuration found, continuing with AP mode only");
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
    } else if (pressDuration >= 3000 && pressDuration < PAIRING_BUTTON_HOLD_TIME) {
      // Long press (3-10s): reset WiFi settings (for future implementation)
      logger.println("Long press detected - WiFi reset requested");
    } else if (pressDuration >= PAIRING_BUTTON_HOLD_TIME) {
      // Very long press (10+ seconds): enter pairing mode
      logger.println("Pairing mode button press detected");
      enterPairingMode();
    }
  }
  
  // Check if button is still being held for pairing mode
  if (currentButtonState && buttonPressed && !deviceState.pairingMode) {
    unsigned long holdDuration = millis() - buttonPressTime;
    if (holdDuration >= PAIRING_BUTTON_HOLD_TIME) {
      logger.println("Entering pairing mode...");
      enterPairingMode();
    }
  }
}

void toggleRelay() {
  deviceState.relayState = !deviceState.relayState;
  digitalWrite(RELAY_PIN, deviceState.relayState ? HIGH : LOW);
  logger.printf("Relay %s\n", deviceState.relayState ? "ON" : "OFF");
  
  // Save state to flash
  saveRelayState();
  
  // Broadcast state change via ESP-NOW
  broadcastDeviceState();
}

void turnOnRelay() {
  if (!deviceState.relayState) {
    deviceState.relayState = true;
    digitalWrite(RELAY_PIN, HIGH);
    logger.println("Relay ON");
    saveRelayState();
    broadcastDeviceState();
  }
}

void turnOffRelay() {
  if (deviceState.relayState) {
    deviceState.relayState = false;
    digitalWrite(RELAY_PIN, LOW);
    logger.println("Relay OFF");
    saveRelayState();
    broadcastDeviceState();
  }
}

void saveRelayState() {
  if (!LittleFS.begin()) {
    logger.println("Failed to mount LittleFS for relay state save");
    return;
  }
  
  File file = LittleFS.open(RELAY_STATE_FILE, "w");
  if (!file) {
    logger.println("Failed to open relay state file for writing");
    return;
  }
  
  // Write relay state as simple boolean (1 byte)
  file.write(deviceState.relayState ? 1 : 0);
  file.close();
  
  logger.printf("Relay state saved: %s\n", deviceState.relayState ? "ON" : "OFF");
}

void loadRelayState() {
  if (!LittleFS.begin()) {
    logger.println("Failed to mount LittleFS for relay state load");
    return;
  }
  
  if (!LittleFS.exists(RELAY_STATE_FILE)) {
    logger.println("No saved relay state found, defaulting to OFF");
    deviceState.relayState = false;
    return;
  }
  
  File file = LittleFS.open(RELAY_STATE_FILE, "r");
  if (!file) {
    logger.println("Failed to open relay state file for reading");
    deviceState.relayState = false;
    return;
  }
  
  // Read relay state (1 byte)
  uint8_t savedState = file.read();
  file.close();
  
  deviceState.relayState = (savedState == 1);
  digitalWrite(RELAY_PIN, deviceState.relayState ? HIGH : LOW);
  
  logger.printf("Relay state loaded: %s\n", deviceState.relayState ? "ON" : "OFF");
  
  // Broadcast the loaded state to other devices
  broadcastDeviceState();
}

void updateSensorReadings() {
  static unsigned long lastReading = 0;
  logger.withoutSerial([]() { //Skip logging to Serial
    if (millis() - lastReading > 1000) {  // Update every second
      logger.println("Calling sensor handle()");
      cse7766.handle();
    
      deviceState.voltage = cse7766.getVoltage();
      deviceState.current = cse7766.getCurrent();
      deviceState.power = cse7766.getActivePower();
      deviceState.energy = cse7766.getEnergy();
      deviceState.lastUpdate = millis();
      
      // Current-based automation for parent devices
      if (deviceState.isParent && deviceState.childCount > 0) {
        bool currentIsHigh = (deviceState.current >= CURRENT_THRESHOLD);
        
        // Check for threshold crossing
        if (currentIsHigh != currentAutomation.lastCurrentState) {
          currentAutomation.lastCurrentState = currentIsHigh;
          sendCurrentAlert(currentIsHigh);
          logger.printf("Parent: Current threshold crossed - sending %s alert to children (%.3fA)\n",
                       currentIsHigh ? "HIGH" : "LOW", deviceState.current);
        }
      }
      
      // Debug output every 10 seconds
      static unsigned long lastDebug = 0;
      if (millis() - lastDebug > 10000) {
        logger.printf("Power: %.2fW, Voltage: %.1fV, Current: %.3fA, Energy: %.2fWh\n",
                     deviceState.power, deviceState.voltage, deviceState.current, deviceState.energy);
        lastDebug = millis();
      }
      
      lastReading = millis();
    }
  }); //end lambda wrapper
}

void updateLEDStatus() {
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  
  unsigned long currentTime = millis();
  unsigned long blinkInterval;
  
  // Determine LED behavior based on device state
  if (deviceState.pairingMode) {
    // Fast blink in pairing mode
    blinkInterval = deviceState.isParent ? PAIRING_LED_SLOW_BLINK : PAIRING_LED_FAST_BLINK;
  } else if (deviceState.wifiConnected) {
    // Solid on when WiFi connected and not pairing
    digitalWrite(LED_PIN, LOW);
    return;
  } else {
    // Normal blink when no WiFi
    blinkInterval = 500;
  }
  
  // Blink LED based on determined interval
  if (currentTime - lastBlink > blinkInterval) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? LOW : HIGH);
    lastBlink = currentTime;
  }
}

void initOTA() {
  // Only enable OTA if WiFi is connected
  if (!deviceState.wifiConnected) {
    Serial.println("OTA: WiFi not connected, skipping OTA setup");
    return;
  }
  
  // Configure OTA settings
  ArduinoOTA.setHostname(HOSTNAME.c_str());
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setPort(OTA_PORT);
  
  // OTA event callbacks
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    
    // Turn off relay during update for safety
    digitalWrite(RELAY_PIN, LOW);
    deviceState.relayState = false;
    
    Serial.println("OTA: Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA: Update completed successfully");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static unsigned long lastProgress = 0;
    unsigned long now = millis();
    
    // Update progress every 2 seconds to avoid spam
    if (now - lastProgress > 2000) {
      Serial.printf("OTA Progress: %u%% (%u/%u)\n", (progress / (total / 100)), progress, total);
      lastProgress = now;
    }
    
    // Blink LED during update
    static bool otaLed = false;
    if (now % 200 == 0) {
      otaLed = !otaLed;
      digitalWrite(LED_PIN, otaLed ? LOW : HIGH);
    }
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
    
    // Restore normal LED operation on error
    digitalWrite(LED_PIN, HIGH);
  });
  
  // Start OTA service
  ArduinoOTA.begin();
  
  Serial.println("OTA: Ready for updates");
  Serial.printf("OTA: Hostname: %s.local\n", ArduinoOTA.getHostname().c_str());
  Serial.printf("OTA: Port: %d\n", OTA_PORT);
  Serial.println("OTA: Use Arduino IDE -> Tools -> Port -> Network Port");
}