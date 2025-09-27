/*
 * Configuration file for SONOFF S31 ESP8266 Project
 * Modify these settings according to your network and preferences
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

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
};

// Hardware pins for SONOFF S31
#define RELAY_PIN 12
#define LED_PIN 13
#define BUTTON_PIN 0
#define CSE7766_RX_PIN 1  // GPIO1 (TX) - CSE7766 TX connects here
#define CSE7766_TX_PIN 3  // GPIO3 (RX) - Not used but defined

// WiFi Configuration
#define WIFI_SSID "YourWiFiNetwork"        // Replace with your WiFi network name
#define WIFI_PASSWORD "YourWiFiPassword"   // Replace with your WiFi password

// Access Point Configuration (for initial setup)
#define AP_SSID "SONOFF-S31-Setup"
#define AP_PASSWORD "sonoff123"

// ESP-NOW Configuration
#define ESPNOW_CHANNEL 1
#define ESPNOW_BROADCAST_INTERVAL 10000    // Broadcast interval in milliseconds
#define MAX_ESPNOW_PEERS 10

// Web Server Configuration
#define WEB_SERVER_PORT 80
#define WEB_UPDATE_INTERVAL 2000           // Web dashboard update interval in milliseconds

// CSE7766 Configuration
#define CSE7766_VOLTAGE_RATIO 1.0          // Voltage calibration ratio
#define CSE7766_CURRENT_RATIO 1.0          // Current calibration ratio
#define CSE7766_POWER_RATIO 1.0            // Power calibration ratio

// Device Configuration
#define DEVICE_NAME "SONOFF S31 Smart Plug"
#define FIRMWARE_VERSION "1.0.0"

// Safety Configuration
#define MAX_POWER_WATTS 3500               // Maximum power limit in watts
#define MAX_CURRENT_AMPS 16                // Maximum current limit in amps
#define OVERHEAT_TEMP_C 80                 // Overheat protection temperature

// Debug Configuration
#define DEBUG_SERIAL 1                     // Enable serial debug output
#define DEBUG_ESPNOW 1                     // Enable ESP-NOW debug output
#define DEBUG_SENSOR 1                     // Enable sensor debug output

#endif // CONFIG_H