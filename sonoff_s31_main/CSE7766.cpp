/*
 * CSE7766 Power Monitoring Sensor Library Implementation
 * For SONOFF S31 ESP8266 Project
 */

#include "config.h"
#include "CSE7766.h"

CSE7766::CSE7766(SoftwareSerial& serial) {
  _serial = &serial;
  _bufferIndex = 0;
  _dataReady = false;
  _lastUpdate = 0;
  
  // Default calibration values
  _voltageCoeff = CSE7766_VOLTAGE_RATIO;
  _currentCoeff = CSE7766_CURRENT_RATIO;
  _powerCoeff = CSE7766_POWER_RATIO;
  
  // Initialize values
  _voltage = 0.0;
  _current = 0.0;
  _power = 0.0;
  _energy = 0.0;
  
  _rawVoltage = 0;
  _rawCurrent = 0;
  _rawPower = 0;
}

void CSE7766::begin() {
  _serial->begin(4800);
  _bufferIndex = 0;
  _dataReady = false;
  
  #if DEBUG_SENSOR
  Serial.println("CSE7766: Sensor initialized");
  #endif
}

bool CSE7766::available() {
  while (_serial->available()) {
    uint8_t byte = _serial->read();
    
    // Look for frame header (0x55)
    if (_bufferIndex == 0 && byte != 0x55) {
      continue;
    }
    
    _buffer[_bufferIndex++] = byte;
    
    // Complete frame received (24 bytes)
    if (_bufferIndex == 24) {
      if (validateChecksum(_buffer, 24)) {
        processPacket();
        _dataReady = true;
        _lastUpdate = millis();
      }
      _bufferIndex = 0;
      return _dataReady;
    }
  }
  
  return false;
}

bool CSE7766::validateChecksum(uint8_t* data, uint8_t length) {
  uint8_t checksum = 0;
  for (int i = 2; i < length - 1; i++) {
    checksum += data[i];
  }
  return (checksum == data[length - 1]);
}

void CSE7766::processPacket() {
  // Extract raw values from the packet
  _rawVoltage = ((uint32_t)_buffer[5] << 16) | ((uint32_t)_buffer[6] << 8) | _buffer[7];
  _rawCurrent = ((uint32_t)_buffer[8] << 16) | ((uint32_t)_buffer[9] << 8) | _buffer[10];
  _rawPower = ((uint32_t)_buffer[11] << 16) | ((uint32_t)_buffer[12] << 8) | _buffer[13];
  
  // Convert raw values to actual measurements
  if (_rawVoltage > 0) {
    _voltage = (_rawVoltage / 1000.0) * _voltageCoeff;
  } else {
    _voltage = 0.0;
  }
  
  if (_rawCurrent > 0) {
    _current = (_rawCurrent / 1000.0) * _currentCoeff;
  } else {
    _current = 0.0;
  }
  
  if (_rawPower > 0) {
    _power = (_rawPower / 1000.0) * _powerCoeff;
  } else {
    _power = 0.0;
  }
  
  #if DEBUG_SENSOR
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 5000) {
    Serial.printf("CSE7766: V=%.1fV, I=%.3fA, P=%.2fW (Raw: %lu, %lu, %lu)\n", 
                  _voltage, _current, _power, _rawVoltage, _rawCurrent, _rawPower);
    lastDebug = millis();
  }
  #endif
}

void CSE7766::update() {
  available();
}

float CSE7766::getVoltage() {
  return _voltage;
}

float CSE7766::getCurrent() {
  return _current;
}

float CSE7766::getActivePower() {
  return _power;
}

float CSE7766::getEnergy() {
  return _energy;
}

void CSE7766::resetEnergy() {
  _energy = 0.0;
}

void CSE7766::setCalibration(float voltage, float current, float power) {
  _voltageCoeff = voltage;
  _currentCoeff = current;
  _powerCoeff = power;
  
  #if DEBUG_SENSOR
  Serial.printf("CSE7766: Calibration set - V:%.3f, I:%.3f, P:%.3f\n", 
                voltage, current, power);
  #endif
}

bool CSE7766::isDataValid() {
  return _dataReady && (millis() - _lastUpdate < 5000);
}

unsigned long CSE7766::getLastUpdate() {
  return _lastUpdate;
}