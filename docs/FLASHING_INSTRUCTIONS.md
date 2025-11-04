# SONOFF S31 Flashing Instructions

## Prerequisites

1. **Hardware Requirements**:
   - SONOFF S31 Smart Plug
   - USB to Serial adapter (3.3V)
   - Jumper wires
   - Soldering iron (for permanent connection)

2. **Software Requirements**:
   - Arduino IDE 1.8.19 or later
   - ESP8266 board package
   - Required libraries (see main README)

## Hardware Preparation

### Option 1: Temporary Connection (Recommended for testing)

1. **Open the SONOFF S31**:
   - Remove the 4 screws from the bottom
   - Carefully separate the case halves
   - Locate the ESP8266 module

2. **Identify Programming Pins**:
   ```
   3V3  - 3.3V Power
   RX   - Connect to TX of USB-Serial adapter
   TX   - Connect to RX of USB-Serial adapter
   GND  - Ground
   ```

3. **Wiring**:
   ```
   SONOFF S31    USB-Serial Adapter
   3V3      ->   3.3V
   GND      ->   GND
   RX       ->   TX
   TX       ->   RX
   ```

### Option 2: Permanent Programming Header (Advanced)

1. **Install Pin Headers**:
   - Solder a 4-pin header to the programming pads
   - This allows easy future programming

2. **Create Programming Cable**:
   - Make a custom cable with the pinout above

## Programming Steps

### 1. Prepare Arduino IDE

1. **Install ESP8266 Board Package**:
   - File → Preferences
   - Add to Additional Board Manager URLs:
     ```
     http://arduino.esp8266.com/stable/package_esp8266com_index.json
     ```
   - Tools → Board → Boards Manager
   - Search "ESP8266" and install

2. **Install Required Libraries**:
   - Sketch → Include Library → Manage Libraries
   - Install: 
     - `ArduinoJson` by Benoit Blanchon
     - `Adafruit MQTT Library` by Adafruit

### 2. Configure Board Settings

```
Board: "Generic ESP8266 Module"
Flash Mode: "DIO"
Flash Size: "1MB (FS:64KB OTA:~470KB)"
Debug Port: "Disabled"
Debug Level: "None"
IwIP Variant: "v2 Lower Memory"
VTables: "Flash"
Exceptions: "Legacy (new can return nullptr)"
Erase Flash: "Only Sketch"
SSL Support: "All SSL ciphers (most compatible)"
CPU Frequency: "80MHz"
Flash Frequency: "40MHz"
Upload Speed: "115200"
```

### 3. Programming Process - initial

1. **Connect Hardware**:
   - Wire SONOFF S31 as described above
   - Hold Power button on SONOFF device

2. **Power On**:
   - Connect USB-Serial adapter to computer
   - ESP8266 should be in programming mode
   - Release Power button on SONOFF device

3. **Upload Code**:
   - Select correct COM port
   - Open `sonoff_s31_main.ino`
   - Click Upload
   - Monitor progress in IDE

4. **Verify Upload**:
   - Open Serial Monitor (115200 baud)
   - Reset the device
   - Should see startup messages

### 4. First Boot Configuration

1. **Initial Setup**:
   - Device creates WiFi AP: "SONOFF-S31-Setup"
   - Password: "sonoff123"
   - Connect to AP with phone/computer

2. **Web Configuration**:
   - Open browser to: `http://192.168.4.1`
   - Configure WiFi credentials via Dashboard
   - Note device name

3. **Post Configuration Operation**:
   - device will be available on your local network at https://sonoff-s31-<unique ID>.local
   - device can now be flashed via Arduino OTA in the IDE

## Troubleshooting

### Common Issues

1. **Upload Failed**:
   - Check wiring connections
   - Ensure Power button is pressed during boot
   - Try lower upload speed (57600)
   - Check power supply (stable 3.3V)

2. **No Serial Output**:
   - Check TX/RX connections (may be swapped)
   - Verify baud rate (115200)
   - Check ground connection

3. **CSE7766 No Data**:
   - Check baud rate (4800 for CSE7766)

### Debug Steps

1. **Enable Debug Output**:
   ```cpp
   #define DEBUG_SERIAL 1
   #define DEBUG_ESPNOW 1
   #define DEBUG_SENSOR 1
   ```


## Safety Warnings

⚠️ **DANGER - HIGH VOLTAGE** ⚠️

- **NEVER** work on the device while connected to mains power
- **ALWAYS** disconnect power before opening the case
- **VERIFY** power is off with a multimeter
- **ONLY** work on the device if you have electrical experience

### Programming Safety

- Use only 3.3V for programming (NOT 5V)
- Double-check wiring before applying power
- Use current-limited power supply if possible
- Never connect mains power during programming

## Restoring Original Firmware

If you need to restore the original SONOFF firmware:

1. **Backup Original Firmware** (before first flash):
   ```bash
   esptool.py --port /dev/ttyUSB0 read_flash 0x00000 0x100000 sonoff_s31_backup.bin
   ```

2. **Restore Original Firmware**:
   ```bash
   esptool.py --port /dev/ttyUSB0 write_flash 0x00000 sonoff_s31_backup.bin
   ```

## Advanced Programming

### OTA Updates

Once the initial firmware is installed:
- Use Arduino IDE OTA programming
- Upload over WiFi network
- No need to open case again
