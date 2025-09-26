/*
 * CSE7766 Power Monitoring Sensor Library
 * For SONOFF S31 ESP8266 Project
 */

#ifndef CSE7766_H
#define CSE7766_H

#include <SoftwareSerial.h>

class CSE7766 {
private:
  SoftwareSerial* _serial;
  uint8_t _buffer[24];
  uint8_t _bufferIndex;
  
  // Calibration coefficients
  float _voltageCoeff;
  float _currentCoeff;
  float _powerCoeff;
  
  // Raw values
  uint32_t _rawVoltage;
  uint32_t _rawCurrent;
  uint32_t _rawPower;
  
  // Calculated values
  float _voltage;
  float _current;
  float _power;
  float _energy;
  
  bool _dataReady;
  unsigned long _lastUpdate;
  
  bool validateChecksum(uint8_t* data, uint8_t length);
  void processPacket();
  
public:
  CSE7766(SoftwareSerial& serial);
  
  void begin();
  bool available();
  void update();
  
  float getVoltage();
  float getCurrent();
  float getActivePower();
  float getEnergy();
  
  void resetEnergy();
  void setCalibration(float voltage, float current, float power);
  
  bool isDataValid();
  unsigned long getLastUpdate();
};

#endif // CSE7766_H