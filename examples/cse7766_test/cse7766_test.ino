/*
 * CSE7766 Sensor Test
 * Use this to test CSE7766 communication and calibrate readings
 */

#include <SoftwareSerial.h>

#define CSE7766_RX_PIN 2  // GPIO2 for testing (use GPIO1 for actual SONOFF S31)
#define CSE7766_TX_PIN 4  // GPIO4 for testing (use GPIO3 for actual SONOFF S31)

SoftwareSerial cseSerial(CSE7766_RX_PIN, CSE7766_TX_PIN);

uint8_t buffer[24];
uint8_t bufferIndex = 0;

void setup() {
  Serial.begin(115200);
  cseSerial.begin(4800);
  
  Serial.println("\n--- CSE7766 Sensor Test ---");
  Serial.println("Waiting for CSE7766 data...");
}

void loop() {
  while (cseSerial.available()) {
    uint8_t byte = cseSerial.read();
    
    // Look for frame header (0x55)
    if (bufferIndex == 0 && byte != 0x55) {
      continue;
    }
    
    buffer[bufferIndex++] = byte;
    
    // Complete frame received (24 bytes)
    if (bufferIndex == 24) {
      if (validateChecksum(buffer, 24)) {
        processPacket();
      } else {
        Serial.println("Checksum error!");
      }
      bufferIndex = 0;
    }
  }
  
  delay(100);
}

bool validateChecksum(uint8_t* data, uint8_t length) {
  uint8_t checksum = 0;
  for (int i = 2; i < length - 1; i++) {
    checksum += data[i];
  }
  return (checksum == data[length - 1]);
}

void processPacket() {
  // Extract raw values from the packet
  uint32_t rawVoltage = ((uint32_t)buffer[5] << 16) | ((uint32_t)buffer[6] << 8) | buffer[7];
  uint32_t rawCurrent = ((uint32_t)buffer[8] << 16) | ((uint32_t)buffer[9] << 8) | buffer[10];
  uint32_t rawPower = ((uint32_t)buffer[11] << 16) | ((uint32_t)buffer[12] << 8) | buffer[13];
  
  // Convert raw values to actual measurements
  float voltage = rawVoltage > 0 ? rawVoltage / 1000.0 : 0.0;
  float current = rawCurrent > 0 ? rawCurrent / 1000.0 : 0.0;
  float power = rawPower > 0 ? rawPower / 1000.0 : 0.0;
  
  Serial.printf("Raw Data: V=%lu, I=%lu, P=%lu\n", rawVoltage, rawCurrent, rawPower);
  Serial.printf("Calculated: V=%.1fV, I=%.3fA, P=%.2fW\n", voltage, current, power);
  Serial.println("---");
  
  // Print raw buffer for debugging
  Serial.print("Raw Buffer: ");
  for (int i = 0; i < 24; i++) {
    Serial.printf("%02X ", buffer[i]);
  }
  Serial.println();
  Serial.println("=====================================");
}