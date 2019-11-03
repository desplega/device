/*
  Require Library: 
  EEPROM Library (https://www.arduino.cc/en/Reference/EEPROM)
  
  Upload Data to IoT Lora Gateway:
  Support Devices: LoRa Radio Node v1.0
  
  NVM map and DEVICE ID code definition

  Copyright: desplega.com
*/

#include <EEPROM.h>
#include "nvm.h"

void nvmInit()
{
  if (EEPROM.read(VIRGIN_ADDR) == 0xFF)
  {
    nvmReset();
  }
}

void nvmReset()
{
  char deviceID[6] = DEVICE_ID;

  // Store deviceID
  for (int i = 0; i < DEVICE_ID_LENGTH; i++)
  {
    EEPROM.write(DEVICE_ID_ADDR + i, deviceID[i]);
  }

  // Mark NVM as already configured
  EEPROM.write(VIRGIN_ADDR, 1);
}

void nvmGetDeviceID(char *deviceID)
{
  // Read deviceID
  for (int i = 0; i < DEVICE_ID_LENGTH; i++)
  {
    deviceID[i] = EEPROM.read(DEVICE_ID_ADDR + i);
  }
}
