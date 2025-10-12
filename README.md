# SONOFF S31 ESP-NOW Auto Remote Switch

## Disclaimer
This project was created with the assistance of Github Copilot in VS Code, using the `Claude Sonnet 4` model.
## 

A decentralized Arduino project that networks 2 or more SONOFF S31 smart plugs using ESP-NOW communication for automatic synchronized control. **no coordinator or hub required**.
When a load connected to the primary unit turns on/off, all secondary units follow suit automatically.

**TODO** Make this multi-sensor, multi-switched to support more use-cases.

## Core Concept

This project transforms multiple SONOFF S31 devices into a **parent-child network** where:
- **Primary (Parent) Device**: Controls the main load and triggers network-wide actions
- **Secondary (Child) Devices**: Automatically mirror the primary device's relay state
- **Decentralized Operation**: No ESPHome, Home Assistant, or other coordinator needed
- **Direct Communication**: Uses ESP-NOW for instant, reliable device-to-device messaging

## Key Features

- **Physical Pairing**: Pair devices without using the web-interface via button sequence
- **Automatic Synchronization**: Child devices instantly follow parent relay state changes
- **Current-Based Automation**: Parent detects 1+ amp load and triggers child devices after 3 seconds
- **ESP-NOW Mesh Network**: Direct device communication without WiFi infrastructure dependency  
- **WiFi Web Dashboard**: Monitor all devices and configure relationships via web interface
- **CSE7766 Power Monitoring**: Real-time voltage, current, power, and energy measurements
- **Persistent Configuration**: Device relationships and settings stored in flash memory
- **Visual Status Indicators**: LED feedback shows device role and network status

## Key Advantages

### No Coordinator Required
- **Fully Decentralized**: Devices communicate directly via ESP-NOW
- **No Single Point of Failure**: Network continues operating even if one device fails
- **No Internet Dependency**: Works completely offline once configured
- **No Hub Hardware**: No need for ESPHome, Home Assistant, or dedicated coordinator

### Instant Response
- **Sub-Second Communication**: ESP-NOW provides near-instant device-to-device messaging
- **Real-Time Automation**: Current detection triggers immediate network response
- **Reliable Operation**: Mesh network automatically handles device connectivity

## Hardware Requirements

- **2 or more SONOFF S31 Smart Plugs** (ESP8266-based)
  - Built-in CSE7766 power monitoring chip (for current detection)
  - Built-in relay and LED indicators
- No additional hardware or coordinator required

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

2. **Flash All Devices**:
   - see [docs/FLASHING_INSTRUCTIONS.md](docs/FLASHING_INSTRUCTIONS.md) for more information
   - Upload the same firmware to all SONOFF S31 devices
   - Each device will auto-generate a unique ID
   - No device-specific configuration needed

3. **Network Setup**:
   - **Option 1**: Out-of-the-box, ESP-NOW is sufficent for pairing and operation
   - **Option 2**: Connect to built-in WIFI access point, visit dashboard and configure local WiFi network credentials, and then restart device.
  

4. **Device Pairing**:
   - Use web interface or button sequence to enter pairing mode
   - Devices automatically discover each other via ESP-NOW
   - Establish parent-child relationships through web dashboard or button sequence

## Required Libraries

Install these libraries via Arduino IDE Library Manager:

```
ESP8266WiFi (built-in)
ESP8266WebServer (built-in)
ESP8266mDNS (built-in)
ArduinoJson by Benoit Blanchon
```

## Configuration

### ESP-NOW Settings (`config.h`)
```cpp
#define ESPNOW_CHANNEL 1                    // WiFi channel for ESP-NOW
#define ESPNOW_BROADCAST_INTERVAL 10000     // Device discovery interval (ms)
#define MAX_ESPNOW_PEERS 10                 // Maximum child devices per parent
#define CURRENT_AUTOMATION_THRESHOLD 1.0    // Amp threshold for automation
#define CHILD_TURN_OFF_DELAY 3000           // Delay before turning on children (ms)
```

### Current Automation Settings
```cpp
#define AUTOMATION_CURRENT_THRESHOLD 1.0    // Minimum amps to trigger children
#define AUTOMATION_DELAY_MS 3000            // Delay before activating children
```

## Web Dashboard

Access the web dashboard at:
- **AP Mode**: `http://192.168.4.1` (when connected to device's AP)
- **mDNS**: `http://sonoff-s31-<unique id>.local` (after WiFi credentials for local network have been configured)

### Dashboard Features

- **Real-time Power Monitoring**: Voltage, current, power, and energy consumption
- **Relay Control**: Toggle power on/off with visual feedback
- **Device Status**: WiFi connection, uptime, and system information
- **ESP-NOW Network**: View and control connected ESP-NOW devices
- **Pairing***: Pair parent-child devices
- **Responsive Design**: Works on mobile and desktop browsers

## How It Works

### Parent-Child Architecture

1. **Parent Device**: 
   - Monitors its current consumption via CSE7766 sensor
   - When load ≥ 1 amp detected, immediately sends "current alert" message to all children
   - When load drops below 1 amp, immediately sends "current alert" message to all children

2. **Child Devices**:
   - Listen for ESP-NOW commands from parent
   - Automatically mirror parent's relay state
   - when a "current alert" message of "HIGH" is received, turn on load
   - when a "current alert" message of "LOW" is received, wait 3 seconds and then turn off
   - Can be manually overridden via web interface or button

3. **Automatic Discovery**:
   - Devices broadcast their presence every 10 seconds
   - Parent devices maintain a list of available children
   - No manual MAC address configuration required

### ESP-NOW Communication

**Message Types:**
- **Status Broadcasts**: Device state, role, and current measurements
- **Relay Commands**: Turn on/off/toggle commands between devices
- **Current Alerts**: When the current of a sensor (parent) device goes high or low, broadcast to all listeners (children)
- **Pairing Messages**: Device discovery and relationship establishment
- **Heartbeat**: Network health monitoring

**Command Format:**
```json
{
  "command": "relay",
  "value": "on|off|toggle",
  "deviceId": "SONOFF_S31_ABC123"
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
```

## Usage

### Quick Start Setup

1. **Flash and Power On**: Upload firmware to all devices, power them on
2. **Automatic Discovery**: Devices will find each other within ~30 seconds
3. **Set Parent Role**: Use web interface to designate one device as "Parent"
4. **Add Children**: Connect other devices as "Children" of the parent
5. **Test**: Plug a 1+ amp load into parent - children should turn on after 3 seconds

### Device Control Methods

**Local Control:**
- **Button Press**: Toggle individual device relay
- **LED Indicators**: Show device role (parent/child) and network status

**Web Dashboard Control:**
1. Connect to any device's web interface (if WiFi configured)
2. View network topology and device relationships
3. Manually control individual devices or entire network
4. Monitor real-time power consumption and device status

**Automatic Operation:**
- Parent device monitors current consumption continuously  
- ≥1 amp load triggers children after 3-second delay
- Load removal immediately turns off all children
- No user intervention required for normal operation

### OTA Updates
1. **Initial Setup**: Device must be connected to WiFi
2. **Arduino IDE**: 
   - Go to Tools → Port
   - Select "sonoff-s31.local" from Network Ports
   - Upload sketch wirelessly
3. **Password**: Use "sonoff123" (configurable in config.h)
4. **Safety**: Relay automatically turns off during updates

## Use Cases

This project is ideal for scenarios where multiple devices need synchronized control without any central coordinator:

### Workshop/Garage Setup
- **Parent**: Table saw or major power tool
- **Children**: Dust collector, shop lighting, air filtration
- **Benefit**: Safety equipment automatically activates when tools are used

### Aquarium/Terrarium System  
- **Parent**: Main circulation pump
- **Children**: Heaters, UV sterilizers, backup pumps, lighting
- **Benefit**: Complete system coordination based on main pump operation

### Home Theater/Entertainment
- **Parent**: TV or audio receiver
- **Children**: Subwoofer, ambient lighting, cooling fans, gaming consoles
- **Benefit**: Entire entertainment system powers up/down together

### Kitchen Appliance Coordination
- **Parent**: Stand mixer or food processor
- **Children**: Under-cabinet task lighting, ventilation hood, scale
- **Benefit**: Work area optimally configured when appliances are active

### Server/Network Rack
- **Parent**: Main server or critical equipment
- **Children**: Cooling fans, network switches, monitoring displays
- **Benefit**: Infrastructure scales with server load automatically

### Art Studio/Workspace
- **Parent**: Kiln, 3D printer, or other primary equipment  
- **Children**: Ventilation, specialized lighting, safety equipment
- **Benefit**: Workspace safety systems activate with equipment use

## Troubleshooting

### Common Issues

1. **Devices Not Finding Each Other**:
   - Check that all devices are powered on and within range (~100m outdoors, ~30m indoors)
   - Verify ESP-NOW channel matches across all devices
   - Wait up to 60 seconds for initial discovery
   - Try power cycling devices if network seems stalled

2. **Parent Not Triggering Children**:
   - Verify parent device is properly configured (check web interface)
   - Ensure load on parent exceeds 1 amp threshold
   - Check that children are properly paired and online
   - Monitor serial output for current readings and ESP-NOW messages

3. **Children Not Responding**:
   - Confirm ESP-NOW communication (check peer list in web interface)
   - Verify child devices have parent relationship established
   - Check for interference from other 2.4GHz devices
   - Try manual relay commands to test communication path

4. **Delayed or Erratic Operation**:
   - Check power supply quality (brownouts can cause issues)
   - Verify current sensor calibration (CSE7766 readings)
   - Monitor for WiFi interference on ESP-NOW channel
   - Consider reducing broadcast interval for faster response

5. **Web Dashboard Issues**:
   - Ensure device has valid IP address (check router or use mDNS)
   - Try connecting directly to AP mode if available
   - Clear browser cache and cookies
   - Check that WiFi credentials are correct in configuration

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

---

**Warning**: This project involves mains electricity. Only attempt modifications if you have proper electrical knowledge and safety equipment. Always disconnect power before making any hardware changes.