/*
  Require Library: 
  EEPROM Library (https://www.arduino.cc/en/Reference/EEPROM)
  
  Upload Data to IoT Lora Gateway:
  Support Devices: LoRa Radio Node v1.0
  
  NVM map and DEVICE ID code definition

  Copyright: desplega.com
*/

// DEVICE ID: To be manually defined with 6 bytes. It must be unique!
#define DEVICE_ID_LENGTH 6
#define DEVICE_ID {19, 11, 03, 18, 12, 00} // Year, Month, Day, Hour, Minutes, Seconds

// NVM map
#define VIRGIN_ADDR 00 // 1 byte
#define DEVICE_ID_ADDR 01 // 6 bytes

void nvmInit();
void nvmReset();
void nvmGetDeviceID(char*);