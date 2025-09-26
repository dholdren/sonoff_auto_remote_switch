/*
 * Web Interface Handler Implementation
 * For SONOFF S31 ESP8266 Project
 */

#include "web_interface.h"
#include "config.h"
#include "espnow_handler.h"

extern ESP8266WebServer server;
extern struct DeviceState deviceState;

void initWebServer() {
  // Web pages
  server.on("/", handleRoot);
  server.on("/style.css", handleStyle);
  server.on("/script.js", handleScript);
  
  // API endpoints
  server.on("/api/status", HTTP_GET, handleGetStatus);
  server.on("/api/relay", HTTP_POST, handleSetRelay);
  server.on("/api/peers", HTTP_GET, handleGetPeers);
  server.on("/api/command", HTTP_POST, handleSendCommand);
  
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.printf("Web server started on port %d\n", WEB_SERVER_PORT);
}

void handleRoot() {
  server.send(200, "text/html", generateWebPage());
}

void handleStyle() {
  String css = R"(
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
)";
  
  server.send(200, "text/css", css);
}

void handleScript() {
  String js = R"(
let statusUpdateInterval;
let peersUpdateInterval;

document.addEventListener('DOMContentLoaded', function() {
  updateStatus();
  updatePeers();
  
  // Update status every 2 seconds
  statusUpdateInterval = setInterval(updateStatus, 2000);
  
  // Update peers every 5 seconds
  peersUpdateInterval = setInterval(updatePeers, 5000);
  
  // Add relay button event listener
  document.getElementById('relayButton').addEventListener('click', toggleRelay);
});

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
)";
  
  server.send(200, "application/javascript", js);
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
  return R"(
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
  )";
}