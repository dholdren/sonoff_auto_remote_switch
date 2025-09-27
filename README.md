# SONOFF S31 ESP8266 Project

A comprehensive Arduino project for the SONOFF S31 smart plug featuring ESP-NOW communication, WiFi web dashboard, and CSE7766 power monitoring sensor integration.

## Features

- **ESP-NOW Communication**: Mesh network capability for device-to-device communication
- **WiFi Web Dashboard**: Real-time monitoring and control via web interface
- **CSE7766 Power Monitoring**: Accurate voltage, current, power, and energy measurements
- **Relay Control**: Local and remote power switching
- **Multi-device Management**: Control multiple ESP-NOW enabled devices
- **Responsive Web Interface**: Works on desktop and mobile devices

## Hardware Requirements

- SONOFF S31 Smart Plug (ESP8266-based)
- Built-in CSE7766 power monitoring chip
- Built-in relay and LED indicators

## Pin Configuration

```
GPIO12 - Relay Control
GPIO13 - LED Indicator
GPIO0  - Button Input
GPIO1  - CSE7766 RX (Hardware Serial TX)
GPIO3  - CSE7766 TX (Hardware Serial RX)
```

## Installation

1. **Setup Arduino IDE**:
   - Install ESP8266 board package
   - Install required libraries (see Libraries section)

2. **Configure Project**:
   - Open `config.h` and update WiFi credentials
   - Adjust calibration values if needed
   - Set device name and other preferences

3. **Upload Code**:
   - Connect SONOFF S31 in programming mode
   - Select "Generic ESP8266 Module" board
   - Upload the sketch

4. **Initial Setup**:
   - Device creates WiFi AP "SONOFF-S31-Setup"
   - Connect and configure via web interface
   - Device will remember settings after reboot

## Required Libraries

Install these libraries via Arduino IDE Library Manager:

```
ESP8266WiFi (built-in)
ESP8266WebServer (built-in)
ESP8266mDNS (built-in)
ArduinoJson by Benoit Blanchon
```

## Configuration

### WiFi Settings (`config.h`)
```cpp
#define WIFI_SSID "YourWiFiNetwork"
#define WIFI_PASSWORD "YourWiFiPassword"
```

### ESP-NOW Settings
```cpp
#define ESPNOW_CHANNEL 1
#define ESPNOW_BROADCAST_INTERVAL 10000
#define MAX_ESPNOW_PEERS 10
```

### CSE7766 Calibration
```cpp
#define CSE7766_VOLTAGE_RATIO 1.0
#define CSE7766_CURRENT_RATIO 1.0
#define CSE7766_POWER_RATIO 1.0
```

## Web Dashboard

Access the web dashboard at:
- **WiFi Mode**: `http://[device-ip]`
- **AP Mode**: `http://192.168.4.1`
- **mDNS**: `http://sonoff-s31.local`

### Dashboard Features

- **Real-time Power Monitoring**: Voltage, current, power, and energy consumption
- **Relay Control**: Toggle power on/off with visual feedback
- **Device Status**: WiFi connection, uptime, and system information
- **ESP-NOW Network**: View and control connected ESP-NOW devices
- **Responsive Design**: Works on mobile and desktop browsers

## ESP-NOW Communication

### Message Types

1. **Device State**: Broadcasts current device status
2. **Commands**: Remote control commands (relay on/off/toggle)
3. **Discovery**: Device discovery and network mapping
4. **Heartbeat**: Keep-alive messages for network health

### API Commands

```json
{
  "command": "relay",
  "value": "on|off|toggle"
}
```

## REST API

### Get Device Status
```
GET /api/status
```

### Control Relay
```
POST /api/relay
Content-Type: application/json

{
  "action": "on|off|toggle"
}
```

### Get ESP-NOW Peers
```
GET /api/peers
```

### Send Command to Peer
```
POST /api/command
Content-Type: application/json

{
  "mac": "AA:BB:CC:DD:EE:FF",
  "command": "relay",
  "value": "toggle"
}
```

## File Structure

```
sonoff_s31_main/
├── sonoff_s31_main.ino    # Main Arduino sketch
├── config.h               # Configuration settings
├── CSE7766.h             # CSE7766 sensor library header
├── CSE7766.cpp           # CSE7766 sensor library implementation
├── espnow_handler.h      # ESP-NOW communication header
├── espnow_handler.cpp    # ESP-NOW communication implementation
├── web_interface.h       # Web server header
├── web_interface.cpp     # Web server implementation
└── README.md            # This file
```

## Usage

### Local Control
- **Short Button Press**: Toggle relay on/off
- **Long Button Press** (3+ seconds): Factory reset (future feature)

### Web Control
1. Connect to device WiFi or ensure on same network
2. Open web browser to device IP
3. Use dashboard to monitor and control device
4. View and control ESP-NOW connected devices

### ESP-NOW Network
- Devices automatically discover each other
- Control multiple devices from any web dashboard
- Mesh network topology for extended range

### OTA Updates
1. **Initial Setup**: Device must be connected to WiFi
2. **Arduino IDE**: 
   - Go to Tools → Port
   - Select "sonoff-s31.local" from Network Ports
   - Upload sketch wirelessly
3. **Password**: Use "sonoff123" (configurable in config.h)
4. **Safety**: Relay automatically turns off during updates

## Troubleshooting

### Common Issues

1. **CSE7766 No Data**:
   - Check serial connection (GPIO1/3)
   - Verify baud rate (4800)
   - Check power supply stability

2. **ESP-NOW Connection Issues**:
   - Ensure devices on same WiFi channel
   - Check distance between devices
   - Verify MAC addresses

3. **Web Dashboard Not Loading**:
   - Check WiFi connection
   - Try AP mode: "SONOFF-S31-Setup"
   - Clear browser cache

4. **Compilation Errors**:
   - Install all required libraries
   - Use ESP8266 board package 3.0.0+
   - Check board settings (Generic ESP8266)

### Debug Output

Enable debug output in `config.h`:
```cpp
#define DEBUG_SERIAL 1
#define DEBUG_ESPNOW 1
#define DEBUG_SENSOR 1
```

## Safety Features

- **Overcurrent Protection**: Automatic relay disconnect at 16A
- **Overpower Protection**: Automatic relay disconnect at 3500W
- **Temperature Monitoring**: Future feature for thermal protection

## Development

### Adding New Features

1. **New Sensor Support**: Add to main loop and web interface
2. **Additional ESP-NOW Commands**: Extend message handlers
3. **Web Interface Enhancements**: Modify web_interface.cpp
4. **Configuration Options**: Add to config.h

### Testing

1. **Hardware Testing**: Use multimeter to verify power readings
2. **Network Testing**: Test ESP-NOW range and reliability
3. **Web Interface**: Test on multiple browsers and devices
4. **Load Testing**: Test with various electrical loads

## License

This project is open source. Feel free to modify and distribute according to your needs.

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Test thoroughly
4. Submit a pull request

## Support

For issues and questions:
1. Check the troubleshooting section
2. Review serial debug output
3. Check hardware connections
4. Verify library versions

## Version History

- **v1.0.0**: Initial release with basic functionality
- Future versions will add OTA updates, advanced scheduling, and more sensors

---

**Warning**: This project involves mains electricity. Only attempt modifications if you have proper electrical knowledge and safety equipment. Always disconnect power before making any hardware changes.