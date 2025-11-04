/*
 * Configuration file for SONOFF S31 ESP8266 Project
 * Modify these settings according to your network and preferences
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ESP-NOW Pairing Configuration Constants
#define MAX_CHILDREN 5                      // Maximum number of child devices

// Device state structure
struct DeviceState {
  bool relayState = false;
  float voltage = 0.0;
  float current = 0.0;
  float power = 0.0;
  float energy = 0.0;
  unsigned long lastUpdate = 0;
  bool wifiConnected = false;
  String deviceId;
  
  // Pairing state
  bool pairingMode = false;
  bool isParent = false;
  bool hasParent = false;
  unsigned long pairingStartTime = 0;
  uint8_t parentMac[6];
  uint8_t childCount = 0;
  uint8_t childMacs[MAX_CHILDREN][6];
};

// Hardware pins for SONOFF S31
#define RELAY_PIN 12
#define LED_PIN 13
#define BUTTON_PIN 0

// WiFi Configuration Structure
struct WiFiConfig {
  char ssid[32];
  char password[64];
  bool isConfigured = false;
};

// Current Automation Structure
struct CurrentAutomation {
  bool lastCurrentState = false;         // Last current threshold state (above/below 1A)
  unsigned long childTurnOffTimer = 0;   // Timer for child turn-off delay
};

// Access Point Configuration (for initial setup)
#define AP_SSID "SONOFF-S31-Setup"
#define AP_PASSWORD "sonoff123"

// ESP-NOW Configuration
#define ESPNOW_CHANNEL 1
#define ESPNOW_BROADCAST_INTERVAL 10000    // Broadcast interval in milliseconds
#define MAX_ESPNOW_PEERS 10

// ESP-NOW Pairing Configuration
#define PAIRING_MODE_TIMEOUT 60000         // Pairing mode timeout in milliseconds
#define PAIRING_BUTTON_HOLD_TIME 10000     // Button hold time to enter pairing mode
#define PAIRING_LED_FAST_BLINK 100         // Fast blink interval for pairing mode
#define PAIRING_LED_SLOW_BLINK 500         // Slow blink interval for parent mode

// Current Automation Configuration
#define CURRENT_THRESHOLD 0.05             // Current threshold in amps for parent automation (lamp with LED light is between .1 and .15)
#define CHILD_TURN_OFF_DELAY 3000          // Delay in milliseconds before child turns off

// Flash Storage Configuration (LittleFS)
#define PAIRING_FILE "/pairing.dat"        // File name for pairing data
#define WIFI_CONFIG_FILE "/wifi.dat"       // File name for WiFi configuration
#define RELAY_STATE_FILE "/relay.dat"      // File name for relay state storage
#define FLASH_MAGIC 0xA5B4                 // Magic number to verify valid data
#define FLASH_VERSION 1

// Pairing data structure for flash storage
struct PairingData {
  uint16_t magic;                       // Magic number for validation
  uint8_t version;                      // Data structure version
  bool isParent;                        // True if this device is a parent
  bool hasParent;                       // True if this device has a parent
  uint8_t parentMac[6];                 // Parent device MAC address
  uint8_t childCount;                   // Number of child devices
  uint8_t childMacs[MAX_CHILDREN][6];   // Child device MAC addresses
  uint32_t checksum;                    // Data integrity checksum
};

// Web Server Configuration
#define WEB_SERVER_PORT 80

// OTA Update Configuration
#define OTA_PASSWORD "sonoff123"            // OTA update password
#define OTA_PORT 8266                       // OTA update port

// Device Configuration
#define DEVICE_NAME "SONOFF S31 Smart Plug"
#define FIRMWARE_VERSION "1.0.0"

// Debug Configuration
#define DEBUG_SERIAL 1                     // Enable/Disable serial debug output
#define DEBUG_ESPNOW 1                     // Enable/Disable ESP-NOW debug output
#define DEBUG_SENSOR 1                     // Enable/Disable sensor debug output
#define DEBUG_MEMORY 0                     // Enable/Disable memory usage debug output

// MQTT Logging
#define MQTT_LOGGING_ENABLED 1
// Using io.adafruit.com (rate limited)
//#define MQTT_SERVER      "io.adafruit.com"
//#define MQTT_SERVERPORT  1883
//#define MQTT_USERNAME    "<username>"
//#define MQTT_PASSWORD    "<key>"
//#define MQTT_TOPIC       AIO_USERNAME "/feeds/sonoff_logging"

//Using local network MQTT (mosquitto)
//Must add this to /etc/mosquitto/mosquitto.conf:
// listener 1883
// allow_anonymous true

//TODO: make this configurable via web interface
#define MQTT_SERVER "192.168.2.103"
#define MQTT_SERVERPORT 1883
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
#define MQTT_TOPIC "sonoff_logging"
#endif  // CONFIG_H