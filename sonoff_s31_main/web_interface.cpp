/*
 * Web Interface Handler Implementation
 * For SONOFF S31 ESP8266 Project
 */

#include "config.h"
#include "web_interface.h"
#include "espnow_handler.h"
#include "Logger.h"
#include <WebSocketsServer.h>
#include <LittleFS.h>

extern ESP8266WebServer server;
extern struct DeviceState deviceState;
extern const char* HOSTNAME;

// WebSocket server for debug logging
WebSocketsServer webSocket = WebSocketsServer(WEBSOCKET_PORT);

// Global WiFi configuration
WiFiConfig wifiConfig;

// WebSocket event handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      logger.printf("WebSocket[%u] Disconnected!\n", num);
      break;
      
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      logger.printf("WebSocket[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      // Send welcome message
      webSocket.sendTXT(num, "{\"type\":\"log\",\"message\":\"WebSocket debug logging connected\\n\"}");
      break;
    }
      
    case WStype_TEXT:
      logger.printf("WebSocket[%u] received text: %s\n", num, payload);
      break;
      
    default:
      break;
  }
}

void initWebServer() {
  // Initialize WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  // Initialize Logger with WebSocket support
  logger.begin(&webSocket);
  
  // Web pages
  server.on("/", handleRoot);
  server.on("/style.css", handleStyle);
  server.on("/script.js", handleScript);
  
  // API endpoints
  server.on("/api/status", HTTP_GET, handleGetStatus);
  server.on("/api/relay", HTTP_POST, handleSetRelay);
  server.on("/api/peers", HTTP_GET, handleGetPeers);
  server.on("/api/command", HTTP_POST, handleSendCommand);
  server.on("/api/pairing", HTTP_POST, handlePairing);
  server.on("/api/wifi", HTTP_GET, handleWiFiConfig);
  server.on("/api/wifi", HTTP_POST, handleSetWiFiConfig);
  
  server.onNotFound(handleNotFound);
  
  server.begin();
  logger.printf("Web server started on port %d\n", WEB_SERVER_PORT);
  logger.printf("WebSocket server started on port %d\n", WEBSOCKET_PORT);
}

void handleRoot() {
  server.send(200, "text/html", generateWebPage());
}

void handleStyle() {
  String css = R"CSSDATA(
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

body {
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  min-height: 100vh;
  color: #333;
}

.container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 20px;
}

.header {
  text-align: center;
  color: white;
  margin-bottom: 30px;
}

.header h1 {
  font-size: 2.5em;
  margin-bottom: 10px;
}

.header p {
  font-size: 1.2em;
  opacity: 0.9;
}

.dashboard {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
  gap: 20px;
  margin-bottom: 30px;
}

.card {
  background: rgba(255, 255, 255, 0.95);
  border-radius: 15px;
  padding: 25px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
  backdrop-filter: blur(10px);
  border: 1px solid rgba(255, 255, 255, 0.2);
}

.card h3 {
  color: #444;
  margin-bottom: 20px;
  font-size: 1.4em;
}

.status-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 15px;
}

.status-item {
  text-align: center;
  padding: 10px;
  background: #f8f9fa;
  border-radius: 8px;
}

.status-item .label {
  font-size: 0.9em;
  color: #666;
  margin-bottom: 5px;
}

.status-item .value {
  font-size: 1.5em;
  font-weight: bold;
  color: #333;
}

.relay-control {
  text-align: center;
}

.relay-button {
  background: linear-gradient(45deg, #4CAF50, #45a049);
  color: white;
  border: none;
  padding: 15px 30px;
  font-size: 1.2em;
  border-radius: 25px;
  cursor: pointer;
  transition: all 0.3s ease;
  box-shadow: 0 4px 15px rgba(76, 175, 80, 0.3);
}

.relay-button:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 20px rgba(76, 175, 80, 0.4);
}

.relay-button.off {
  background: linear-gradient(45deg, #f44336, #d32f2f);
  box-shadow: 0 4px 15px rgba(244, 67, 54, 0.3);
}

.relay-button.off:hover {
  box-shadow: 0 6px 20px rgba(244, 67, 54, 0.4);
}

.peers-list {
  max-height: 300px;
  overflow-y: auto;
}

.peer-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 10px;
  margin-bottom: 10px;
  background: #f8f9fa;
  border-radius: 8px;
  border-left: 4px solid #4CAF50;
}

.peer-item.offline {
  border-left-color: #f44336;
  opacity: 0.6;
}

.peer-info {
  flex-grow: 1;
}

.peer-name {
  font-weight: bold;
  margin-bottom: 3px;
}

.peer-mac {
  font-size: 0.8em;
  color: #666;
}

.peer-controls button {
  background: #2196F3;
  color: white;
  border: none;
  padding: 5px 10px;
  border-radius: 4px;
  cursor: pointer;
  margin-left: 5px;
  font-size: 0.8em;
}

.peer-controls button:hover {
  background: #1976D2;
}

.status-indicator {
  display: inline-block;
  width: 10px;
  height: 10px;
  border-radius: 50%;
  margin-right: 8px;
}

.status-indicator.online {
  background: #4CAF50;
}

.status-indicator.offline {
  background: #f44336;
}

@media (max-width: 768px) {
  .container {
    padding: 10px;
  }
  
  .header h1 {
    font-size: 2em;
  }
  
  .status-grid {
    grid-template-columns: 1fr;
  }
}

.loading {
  display: inline-block;
  width: 20px;
  height: 20px;
  border: 3px solid #f3f3f3;
  border-top: 3px solid #3498db;
  border-radius: 50%;
  animation: spin 1s linear infinite;
}

@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}
)CSSDATA";
  
  server.send(200, "text/css", css);
}

void handleScript() {
  String js = R"JSDATA(
let statusUpdateInterval;
let peersUpdateInterval;
let debugSocket;

document.addEventListener('DOMContentLoaded', function() {
  updateStatus();
  updatePeers();
  updateWiFiStatus();
  initDebugSocket();
  
  // Update status every 2 seconds
  statusUpdateInterval = setInterval(updateStatus, 2000);
  
  // Update peers every 5 seconds
  peersUpdateInterval = setInterval(updatePeers, 5000);
  
  // Update WiFi status every 10 seconds
  setInterval(updateWiFiStatus, 10000);
  
  // Add relay button event listener
  document.getElementById('relayButton').addEventListener('click', toggleRelay);
});

function initDebugSocket() {
  const wsPort = window.location.port === '80' || window.location.port === '' ? '81' : (parseInt(window.location.port) + 1);
  const wsUrl = 'ws://' + window.location.hostname + ':' + wsPort + '/';
  
  debugSocket = new WebSocket(wsUrl);
  
  debugSocket.onopen = function(event) {
    console.log('Debug WebSocket connected');
  };
  
  debugSocket.onmessage = function(event) {
    try {
      const data = JSON.parse(event.data);
      if (data.type === 'log') {
        console.log('[ESP8266]', data.message.trim());
      }
    } catch (e) {
      console.log('[ESP8266 Raw]', event.data);
    }
  };
  
  debugSocket.onclose = function(event) {
    console.log('Debug WebSocket disconnected, attempting to reconnect in 3 seconds...');
    setTimeout(initDebugSocket, 3000);
  };
  
  debugSocket.onerror = function(error) {
    console.error('WebSocket error:', error);
  };
}

async function updateStatus() {
  try {
    const response = await fetch('/api/status');
    const data = await response.json();
    
    // Update status values
    document.getElementById('voltage').textContent = data.voltage.toFixed(1) + 'V';
    document.getElementById('current').textContent = data.current.toFixed(3) + 'A';
    document.getElementById('power').textContent = data.power.toFixed(2) + 'W';
    document.getElementById('energy').textContent = data.energy.toFixed(2) + 'Wh';
    
    // Update WiFi status
    const wifiStatus = document.getElementById('wifiStatus');
    wifiStatus.innerHTML = data.wifi ? 
      '<span class="status-indicator online"></span>Connected' : 
      '<span class="status-indicator offline"></span>Disconnected';
    
    // Update uptime
    const uptime = Math.floor(data.uptime / 1000);
    const hours = Math.floor(uptime / 3600);
    const minutes = Math.floor((uptime % 3600) / 60);
    const seconds = uptime % 60;
    document.getElementById('uptime').textContent = 
      `${hours}h ${minutes}m ${seconds}s`;
    
    // Update firmware version
    document.getElementById('firmware').textContent = data.firmwareVersion || 'Unknown';
    
    // Update OTA status
    const otaStatus = document.getElementById('otaStatus');
    if (data.otaEnabled) {
      otaStatus.innerHTML = `<span class="status-indicator online"></span>Ready (${data.otaHostname})`;
    } else {
      otaStatus.innerHTML = '<span class="status-indicator offline"></span>Disabled';
    }
    
    // Update pairing status
    const pairingMode = document.getElementById('pairingMode');
    if (data.pairingMode) {
      pairingMode.innerHTML = '<span class="status-indicator online"></span>Active';
    } else {
      pairingMode.innerHTML = '<span class="status-indicator offline"></span>Inactive';
    }
    
    // Update device role
    const deviceRole = document.getElementById('deviceRole');
    if (data.isParent) {
      deviceRole.innerHTML = '<span class="status-indicator online"></span>Parent';
    } else if (data.hasParent) {
      deviceRole.innerHTML = '<span class="status-indicator online"></span>Child';
    } else {
      deviceRole.innerHTML = '<span class="status-indicator offline"></span>Standalone';
    }
    
    // Update parent device
    const parentDevice = document.getElementById('parentDevice');
    if (data.hasParent && data.parentMac) {
      parentDevice.textContent = data.parentMac;
    } else {
      parentDevice.textContent = 'None';
    }
    
    // Update child devices
    const childDevices = document.getElementById('childDevices');
    if (data.childCount > 0) {
      childDevices.textContent = `${data.childCount} connected`;
    } else {
      childDevices.textContent = 'None';
    }
    
    // Update relay button
    const relayButton = document.getElementById('relayButton');
    relayButton.textContent = data.relay ? 'Turn OFF' : 'Turn ON';
    relayButton.className = data.relay ? 'relay-button off' : 'relay-button';
    
  } catch (error) {
    console.error('Error updating status:', error);
  }
}

async function updatePeers() {
  try {
    const response = await fetch('/api/peers');
    const data = await response.json();
    
    const peersList = document.getElementById('peersList');
    
    if (data.peers.length === 0) {
      peersList.innerHTML = '<p>No ESP-NOW peers found</p>';
      return;
    }
    
    let html = '';
    data.peers.forEach(peer => {
      const statusClass = peer.online ? 'online' : 'offline';
      const statusText = peer.online ? 'Online' : 'Offline';
      
      html += `
        <div class="peer-item ${statusClass}">
          <div class="peer-info">
            <div class="peer-name">${peer.deviceId || 'Unknown Device'}</div>
            <div class="peer-mac">${peer.mac}</div>
            <div class="peer-status">
              <span class="status-indicator ${statusClass}"></span>${statusText}
            </div>
          </div>
          <div class="peer-controls">
            <button onclick="sendCommand('${peer.mac}', 'relay', 'on')">ON</button>
            <button onclick="sendCommand('${peer.mac}', 'relay', 'off')">OFF</button>
            <button onclick="sendCommand('${peer.mac}', 'relay', 'toggle')">Toggle</button>
          </div>
        </div>
      `;
    });
    
    peersList.innerHTML = html;
    
  } catch (error) {
    console.error('Error updating peers:', error);
  }
}

async function toggleRelay() {
  const button = document.getElementById('relayButton');
  const originalText = button.textContent;
  button.innerHTML = '<span class="loading"></span> Please wait...';
  button.disabled = true;
  
  try {
    const response = await fetch('/api/relay', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ action: 'toggle' })
    });
    
    if (response.ok) {
      // Status will be updated by the regular update interval
      setTimeout(() => {
        button.disabled = false;
      }, 1000);
    } else {
      throw new Error('Failed to toggle relay');
    }
    
  } catch (error) {
    console.error('Error toggling relay:', error);
    button.textContent = originalText;
    button.disabled = false;
    alert('Error toggling relay. Please try again.');
  }
}

async function sendCommand(mac, command, value) {
  try {
    const response = await fetch('/api/command', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        mac: mac,
        command: command,
        value: value
      })
    });
    
    if (response.ok) {
      console.log(`Command sent to ${mac}: ${command}=${value}`);
    } else {
      throw new Error('Failed to send command');
    }
    
  } catch (error) {
    console.error('Error sending command:', error);
    alert('Error sending command. Please try again.');
  }
}

async function enterPairingMode() {
  if (confirm('Enter pairing mode? Device will listen for parent/child devices.')) {
    try {
      const response = await fetch('/api/pairing', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ action: 'enter' })
      });
      
      if (response.ok) {
        alert('Pairing mode activated! LED will blink to indicate status.');
      } else {
        throw new Error('Failed to enter pairing mode');
      }
    } catch (error) {
      console.error('Error entering pairing mode:', error);
      alert('Error entering pairing mode. Please try again.');
    }
  }
}

async function clearPairingData() {
  if (confirm('Clear all pairing data? This will remove parent/child relationships.')) {
    try {
      const response = await fetch('/api/pairing', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ action: 'clear' })
      });
      
      if (response.ok) {
        alert('Pairing data cleared successfully!');
      } else {
        throw new Error('Failed to clear pairing data');
      }
    } catch (error) {
      console.error('Error clearing pairing data:', error);
      alert('Error clearing pairing data. Please try again.');
    }
  }
}

async function updateWiFiStatus() {
  try {
    const response = await fetch('/api/wifi');
    const data = await response.json();
    
    document.getElementById('currentSSID').textContent = data.configured ? data.ssid : 'Not configured';
    document.getElementById('wifiStatus').innerHTML = data.connected ? 
      '<span class="status-indicator online"></span>Connected' : 
      '<span class="status-indicator offline"></span>Disconnected';
  } catch (error) {
    console.error('Error fetching WiFi status:', error);
  }
}

async function updateWiFiConfig() {
  const ssid = document.getElementById('newSSID').value.trim();
  const password = document.getElementById('newPassword').value;
  
  if (!ssid) {
    alert('Please enter a WiFi network name (SSID)');
    return;
  }
  
  if (ssid.length > 31) {
    alert('WiFi network name too long (max 31 characters)');
    return;
  }
  
  if (password.length > 63) {
    alert('WiFi password too long (max 63 characters)');
    return;
  }
  
  try {
    const response = await fetch('/api/wifi', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ ssid: ssid, password: password })
    });
    
    const result = await response.json();
    
    if (result.success) {
      alert(result.message);
      document.getElementById('newSSID').value = '';
      document.getElementById('newPassword').value = '';
      updateWiFiStatus();
    } else {
      alert('Error: ' + result.message);
    }
  } catch (error) {
    console.error('Error updating WiFi config:', error);
    alert('Error updating WiFi configuration. Please try again.');
  }
}
)JSDATA";
  
  server.send(200, "application/javascript", js);
}

void handleWebSocket() {
  webSocket.loop();
}

void handleGetStatus() {
  server.send(200, "application/json", getStatusJSON());
}

void handleSetRelay() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(200);
    deserializeJson(doc, server.arg("plain"));
    
    String action = doc["action"];
    
    if (action == "on" || action == "1") {
      deviceState.relayState = true;
      digitalWrite(RELAY_PIN, HIGH);
    } else if (action == "off" || action == "0") {
      deviceState.relayState = false;
      digitalWrite(RELAY_PIN, LOW);
    } else if (action == "toggle") {
      deviceState.relayState = !deviceState.relayState;
      digitalWrite(RELAY_PIN, deviceState.relayState ? HIGH : LOW);
    }
    
    // Broadcast state change
    broadcastDeviceState();
    
    server.send(200, "application/json", getStatusJSON());
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid request\"}");
  }
}

void handleGetPeers() {
  server.send(200, "application/json", getPeersJSON());
}

void handleSendCommand() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(200);
    deserializeJson(doc, server.arg("plain"));
    
    String macStr = doc["mac"];
    String command = doc["command"];
    String value = doc["value"];
    
    uint8_t mac[6];
    stringToMac(macStr, mac);
    
    sendCommand(mac, command, value);
    
    server.send(200, "application/json", "{\"status\":\"success\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid request\"}");
  }
}

void handlePairing() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(200);
    deserializeJson(doc, server.arg("plain"));
    
    String action = doc["action"];
    
    if (action == "enter") {
      enterPairingMode();
      server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Pairing mode activated\"}");
    } else if (action == "clear") {
      clearPairingData();
      savePairingData(); // Save cleared state
      server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Pairing data cleared\"}");
    } else {
      server.send(400, "application/json", "{\"error\":\"Invalid action\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid request\"}");
  }
}

void handleNotFound() {
  server.send(404, "text/plain", "File Not Found");
}

String getStatusJSON() {
  DynamicJsonDocument doc(300);
  
  doc["deviceId"] = deviceState.deviceId;
  doc["relay"] = deviceState.relayState;
  doc["voltage"] = deviceState.voltage;
  doc["current"] = deviceState.current;
  doc["power"] = deviceState.power;
  doc["energy"] = deviceState.energy;
  doc["wifi"] = deviceState.wifiConnected;
  doc["uptime"] = millis();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["chipId"] = ESP.getChipId();
  doc["otaEnabled"] = deviceState.wifiConnected;
  doc["otaHostname"] = String(HOSTNAME) + ".local";
  doc["firmwareVersion"] = FIRMWARE_VERSION;
  doc["pairingMode"] = deviceState.pairingMode;
  doc["isParent"] = deviceState.isParent;
  doc["hasParent"] = deviceState.hasParent;
  doc["childCount"] = deviceState.childCount;
  
  if (deviceState.hasParent) {
    doc["parentMac"] = macToString(deviceState.parentMac);
  }
  
  JsonArray children = doc.createNestedArray("children");
  for (int i = 0; i < deviceState.childCount; i++) {
    children.add(macToString(deviceState.childMacs[i]));
  }
  
  String output;
  serializeJson(doc, output);
  return output;
}

String getPeersJSON() {
  DynamicJsonDocument doc(1000);
  JsonArray peers = doc.createNestedArray("peers");
  
  for (int i = 0; i < espnowPeerCount; i++) {
    JsonObject peer = peers.createNestedObject();
    peer["mac"] = macToString(espnowPeers[i].mac);
    peer["deviceId"] = espnowPeers[i].deviceId;
    peer["online"] = espnowPeers[i].isOnline;
    peer["lastSeen"] = espnowPeers[i].lastSeen;
  }
  
  String output;
  serializeJson(doc, output);
  return output;
}

String generateWebPage() {
  String html = R"HTMLDATA(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SONOFF S31 Smart Plug Dashboard</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>SONOFF S31 Smart Plug</h1>
            <p>ESP8266 with ESP-NOW & Power Monitoring</p>
        </div>
        
        <div class="dashboard">
            <!-- Power Monitoring Card -->
            <div class="card">
                <h3>Power Monitoring</h3>
                <div class="status-grid">
                    <div class="status-item">
                        <div class="label">Voltage</div>
                        <div class="value" id="voltage">---V</div>
                    </div>
                    <div class="status-item">
                        <div class="label">Current</div>
                        <div class="value" id="current">---A</div>
                    </div>
                    <div class="status-item">
                        <div class="label">Power</div>
                        <div class="value" id="power">---W</div>
                    </div>
                    <div class="status-item">
                        <div class="label">Energy</div>
                        <div class="value" id="energy">---Wh</div>
                    </div>
                </div>
            </div>
            
            <!-- Device Control Card -->
            <div class="card">
                <h3>Device Control</h3>
                <div class="relay-control">
                    <button id="relayButton" class="relay-button">Loading...</button>
                </div>
                <div style="margin-top: 20px;">
                    <div class="status-item">
                        <div class="label">WiFi Status</div>
                        <div class="value" id="wifiStatus">Checking...</div>
                    </div>
                    <div class="status-item" style="margin-top: 10px;">
                        <div class="label">Uptime</div>
                        <div class="value" id="uptime">---</div>
                    </div>
                    <div class="status-item" style="margin-top: 10px;">
                        <div class="label">Firmware</div>
                        <div class="value" id="firmware">---</div>
                    </div>
                    <div class="status-item" style="margin-top: 10px;">
                        <div class="label">OTA Updates</div>
                        <div class="value" id="otaStatus">---</div>
                    </div>
                </div>
            </div>
            
            <!-- Pairing Status Card -->
            <div class="card">
                <h3>Device Pairing</h3>
                <div class="status-grid">
                    <div class="status-item">
                        <div class="label">Pairing Mode</div>
                        <div class="value" id="pairingMode">---</div>
                    </div>
                    <div class="status-item">
                        <div class="label">Device Role</div>
                        <div class="value" id="deviceRole">---</div>
                    </div>
                    <div class="status-item">
                        <div class="label">Parent Device</div>
                        <div class="value" id="parentDevice">---</div>
                    </div>
                    <div class="status-item">
                        <div class="label">Child Devices</div>
                        <div class="value" id="childDevices">---</div>
                    </div>
                </div>
                <div style="margin-top: 20px; text-align: center;">
                    <button onclick="enterPairingMode()" class="relay-button" style="background: linear-gradient(45deg, #FF9800, #F57C00); margin-right: 10px;">Enter Pairing Mode</button>
                    <button onclick="clearPairingData()" class="relay-button" style="background: linear-gradient(45deg, #f44336, #d32f2f);">Clear Pairing Data</button>
                </div>
            </div>
            
            <!-- WiFi Configuration Card -->
            <div class="card">
                <h3>WiFi Configuration</h3>
                <div class="status-grid">
                    <div class="status-item">
                        <span class="label">Current SSID:</span>
                        <span id="currentSSID">Loading...</span>
                    </div>
                    <div class="status-item">
                        <span class="label">Status:</span>
                        <span id="wifiStatus">Loading...</span>
                    </div>
                </div>
                <div style="margin-top: 20px;">
                    <div style="margin-bottom: 15px;">
                        <label for="newSSID" style="display: block; margin-bottom: 5px; font-weight: bold;">New SSID:</label>
                        <input type="text" id="newSSID" placeholder="Enter WiFi network name" 
                               style="width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; font-size: 14px;">
                    </div>
                    <div style="margin-bottom: 15px;">
                        <label for="newPassword" style="display: block; margin-bottom: 5px; font-weight: bold;">Password:</label>
                        <input type="password" id="newPassword" placeholder="Enter WiFi password" 
                               style="width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; font-size: 14px;">
                    </div>
                    <button onclick="updateWiFiConfig()" class="relay-button" style="background: linear-gradient(45deg, #2196F3, #1976D2);">
                        Update WiFi Configuration
                    </button>
                </div>
            </div>
            
            <!-- ESP-NOW Peers Card -->
            <div class="card">
                <h3>ESP-NOW Network</h3>
                <div id="peersList" class="peers-list">
                    <p>Loading peers...</p>
                </div>
            </div>
        </div>
    </div>
    
        <script src="/script.js"></script>
</body>
</html>
  )HTMLDATA";
  
  return html;
}

// ===== WIFI CONFIGURATION FUNCTIONS =====

void loadWiFiConfig() {
  // Check if WiFi config file exists
  if (!LittleFS.exists(WIFI_CONFIG_FILE)) {
    logger.println("No WiFi config file found - using defaults");
    wifiConfig.isConfigured = false;
    strcpy(wifiConfig.ssid, "");
    strcpy(wifiConfig.password, "");
    return;
  }
  
  // Read from LittleFS
  File file = LittleFS.open(WIFI_CONFIG_FILE, "r");
  if (!file) {
    logger.println("Failed to open WiFi config file for reading");
    wifiConfig.isConfigured = false;
    return;
  }
  
  if (file.size() != sizeof(WiFiConfig)) {
    logger.println("WiFi config file size mismatch");
    file.close();
    wifiConfig.isConfigured = false;
    return;
  }
  
  file.read((uint8_t*)&wifiConfig, sizeof(WiFiConfig));
  file.close();
  
  logger.printf("WiFi config loaded: SSID='%s'\n", wifiConfig.ssid);
}

void saveWiFiConfig() {
  // Write to LittleFS
  File file = LittleFS.open(WIFI_CONFIG_FILE, "w");
  if (file) {
    file.write((uint8_t*)&wifiConfig, sizeof(WiFiConfig));
    file.close();
    logger.printf("WiFi config saved: SSID='%s'\n", wifiConfig.ssid);
  } else {
    logger.println("Failed to open WiFi config file for writing");
  }
}

void clearWiFiConfig() {
  // Remove WiFi config file from flash storage
  if (LittleFS.exists(WIFI_CONFIG_FILE)) {
    LittleFS.remove(WIFI_CONFIG_FILE);
    logger.println("WiFi config file removed from flash storage");
  }
  
  wifiConfig.isConfigured = false;
  strcpy(wifiConfig.ssid, "");
  strcpy(wifiConfig.password, "");
  logger.println("WiFi configuration cleared");
}

void handleWiFiConfig() {
  DynamicJsonDocument doc(200);
  doc["ssid"] = wifiConfig.ssid;
  doc["configured"] = wifiConfig.isConfigured;
  doc["connected"] = deviceState.wifiConnected;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSetWiFiConfig() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(200);
    deserializeJson(doc, server.arg("plain"));
    
    String newSSID = doc["ssid"];
    String newPassword = doc["password"];
    
    if (newSSID.length() > 0 && newSSID.length() < 32 && 
        newPassword.length() >= 0 && newPassword.length() < 64) {
      
      // Update WiFi configuration
      strcpy(wifiConfig.ssid, newSSID.c_str());
      strcpy(wifiConfig.password, newPassword.c_str());
      wifiConfig.isConfigured = true;
      
      // Save to flash
      saveWiFiConfig();
      
      logger.printf("WiFi config updated: SSID='%s'\n", wifiConfig.ssid);
      
      // Respond with success
      DynamicJsonDocument response(200);
      response["success"] = true;
      response["message"] = "WiFi configuration saved. Restart device to apply changes.";
      
      String responseStr;
      serializeJson(response, responseStr);
      server.send(200, "application/json", responseStr);
      
    } else {
      // Invalid parameters
      DynamicJsonDocument response(200);
      response["success"] = false;
      response["message"] = "Invalid SSID or password length";
      
      String responseStr;
      serializeJson(response, responseStr);
      server.send(400, "application/json", responseStr);
    }
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"No data received\"}");
  }
}