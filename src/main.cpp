/*
  Require Library: 
  https://github.com/dragino/RadioHead
  
  Upload Data to IoT Lora Gateway:
  Support Devices: LoRa Radio Node v1.0
  
  This program reads Temperature from Dallas Temperature sensor DS18B20.
  It also reads an analog input that will be connected to a high voltage harp.
  Then send the value to LoRa Gateway, the LoRa Gateway will send the value to the 
  IoT server (MQTT Server)

  Copyright: desplega.com
*/

#include <SPI.h>
#include <RH_RF95.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include "nvm.h"
#include "powerDown.h"

// Store deviceID in a permanent variable to avoid reading nvm everytime we need it
char deviceID[6];

RH_RF95 rf95;

// LoRa data configuration
char dt_dat[5];            // Store Sensor Data (MAX 2 devices!)
#define NODE_ID_LENGTH 6
const char *node_id = "<1234>";  // LoRa End Node ID
float frequency = 868.0;
unsigned int count = 1;

// To construct the LoRa packet we expect always a specific number of devices
#define PACKET_EXPECTED_DEVICES 2

// Harp status input (analog)
#define HARP_INPUT A0

// Data wire is plugged into port 4 on the Arduino
#define ONE_WIRE_BUS 4          // D4 (not the pin number, but the number of digital I/O port)
#define TEMPERATURE_PRECISION 9 // Lower resolution

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

int numberOfDevices; // Number of temperature devices found

DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address

// Function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16)
      Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void initDT(void)
{
  // Start up the library
  sensors.begin();

  // locate devices on the bus
  Serial.print("Locating DT devices... ");

  // Grab a count of devices on the wire
  numberOfDevices = sensors.getDeviceCount();
  Serial.print(numberOfDevices, DEC);
  Serial.println(" device(s) found");

  // Report parasite power requirements
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode())
    Serial.println("ON");
  else
    Serial.println("OFF");

  // Loop through each device, print out address
  for (int i = 0; i < numberOfDevices; i++)
  {
    // Search the wire for address
    if (sensors.getAddress(tempDeviceAddress, i))
    {
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();

      Serial.print("Setting resolution to ");
      Serial.println(TEMPERATURE_PRECISION, DEC);

      // Set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
      sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);

      Serial.print("Resolution actually set to: ");
      Serial.print(sensors.getResolution(tempDeviceAddress), DEC);
      Serial.println();
    }
    else
    {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.println(" but could not detect address. Check power and cabling");
    }
  }
}

uint8_t getHarpStatus(void)
{
  int analogPin = A0;
  // If Vh > 0,8V then Harp is On
  int analogValue = analogRead(analogPin);
  Serial.print("Vh = ");
  Serial.println(analogValue);
  return (analogValue > 250 ? 1 : 0);  
}

void initLoRa(void)
{
  // LoRa init
  if (!rf95.init())
    Serial.println("init failed");
  // Setup ISM frequency
  rf95.setFrequency(frequency);
  // Setup Power,dBm
  rf95.setTxPower(13);
  rf95.setSyncWord(0x34);

  Serial.println("Dallas Temperature IC Control & LoRa Demo");
  Serial.print("LoRa End Node ID: ");

  Serial.print(node_id);
  Serial.println();
}

void setup()
{
  // Configure serial port
  Serial.begin(9600);

  // Init sleep mode
  initSleep();

  // Init NVM
  nvmInit();
  nvmGetDeviceID(deviceID);

  // Temperature sensors init
  initDT();

  // LoRa Init
  initLoRa();

  // Start in idle mode, required to avoid continuous reset... but I don't know why :(
  delay(1000); // Wait some time just in case to make everything stable
  rf95.sleep();
  goToSleep(INIT_SLEEP_TIME);
}

void readData()
{
  // Get data from temperature sensors (up to 2 devices)
  if (numberOfDevices > 0)
  {
    sensors.requestTemperatures(); // Send the command to get temperature readings
    for (int i = 0; i < numberOfDevices; i++)
    {
      // Search the wire for address
      if (sensors.getAddress(tempDeviceAddress, i))
      {
        // Get temperature for each device
        float tempC = sensors.getTempC(tempDeviceAddress);
        dt_dat[i * 2] = (int)tempC;                              // Get temperature integer part
        dt_dat[(i * 2) + 1] = (int)((tempC - (int)tempC) * 100); // Get temperature decimal part
      }
      else
      {
        // Ghost device! Check your power requirements and cabling
        Serial.println("Ghost device!");
      }
    }
  }

  // Fill with dummy data in case some sensors fail or are not present
  switch (numberOfDevices)
  {
  case 0:
    dt_dat[0] = 0;
    dt_dat[1] = 0;
    // No break
  case 1:
    dt_dat[2] = 0;
    dt_dat[3] = 0;
    break;
  }

  // Get harp status
  dt_dat[4] = getHarpStatus();
};

uint16_t calcByte(uint16_t crc, uint8_t b)
{
  uint32_t i;
  crc = crc ^ (uint32_t)b << 8;

  for (i = 0; i < 8; i++)
  {
    if ((crc & 0x8000) == 0x8000)
      crc = crc << 1 ^ 0x1021;
    else
      crc = crc << 1;
  }
  return crc & 0xffff;
}

uint16_t CRC16(uint8_t *pBuffer, uint32_t length)
{
  uint16_t wCRC16 = 0;
  uint32_t i;
  if ((pBuffer == 0) || (length == 0))
  {
    return 0;
  }
  for (i = 0; i < length; i++)
  {
    wCRC16 = calcByte(wCRC16, pBuffer[i]);
  }
  return wCRC16;
}

void loop()
{
  Serial.print("###########    ");
  Serial.print("COUNT=");
  Serial.print(count);
  Serial.println("    ###########");
  count++;
  readData();
  char data[50] = {0};
  int dataLength = NODE_ID_LENGTH + DEVICE_ID_LENGTH + PACKET_EXPECTED_DEVICES * 2 + 1; // Payload Length: node_id + temperature(s) + harp

  // Add to data[x] the Node ID
  for (int i = 0; i < NODE_ID_LENGTH; i++) {
    data[i] = node_id[i];
  }

  // Get device ID
  for (int i = 0; i < DEVICE_ID_LENGTH; i++)
  {
    data[NODE_ID_LENGTH + i] = deviceID[i];
  }
  Serial.print("Device ID: ");
  // Convert node ID to text
  char str[25];
  sprintf(str, "%02d %02d %02d %02d %02d %02d", deviceID[0], deviceID[1], deviceID[2], deviceID[3], deviceID[4], deviceID[5]);
  str[24] = 0x00; // End of string
  Serial.println(str);

  // Get temperature(s)
  for (int i = 0; i < PACKET_EXPECTED_DEVICES; i++)
  {
    data[NODE_ID_LENGTH + DEVICE_ID_LENGTH + (i * 2)] = dt_dat[i * 2];        //Get Temperature Integer Part
    data[NODE_ID_LENGTH + DEVICE_ID_LENGTH + 1 + (i * 2)] = dt_dat[(i * 2) + 1]; //Get Temperature Decimal Part
  }

  // Get harp status
  data[NODE_ID_LENGTH + DEVICE_ID_LENGTH + PACKET_EXPECTED_DEVICES * 2] = dt_dat[4]; // Harp status

  // Print temperature(s)
  for (int i = 0; i < PACKET_EXPECTED_DEVICES; i++)
  {
    Serial.print("Temperature ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(data[NODE_ID_LENGTH + DEVICE_ID_LENGTH + (i * 2)], DEC); //Show temperature
    Serial.print(",");
    Serial.print(data[NODE_ID_LENGTH + DEVICE_ID_LENGTH + 1 + (i * 2)], DEC); //Show temperature
    Serial.println("C");
  }
  // Print harp status
  Serial.print("Harp status: ");
  Serial.println(data[NODE_ID_LENGTH + DEVICE_ID_LENGTH + PACKET_EXPECTED_DEVICES * 2], DEC); // Show harp status

  uint16_t crcData = CRC16((unsigned char *)data, dataLength); //get CRC DATA
  //Serial.println(crcData,HEX);

  Serial.print("Data to be sent(without CRC): ");

  for (int i = 0; i < dataLength; i++)
  {
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  unsigned char sendBuf[100] = {0};

  for (int i = 0; i < dataLength; i++)
  {
    sendBuf[i] = data[i];
  }

  sendBuf[dataLength] = (unsigned char)crcData;            // Add CRC to LoRa Data
  sendBuf[dataLength + 1] = (unsigned char)(crcData >> 8); // Add CRC to LoRa Data
  
  Serial.print("Data to be sent(with CRC):    ");
  for (int i = 0; i < (dataLength + 2); i++)
  {
    Serial.print(sendBuf[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  //rf95.send(sendBuf, dataLength + 2); //Send LoRa Data

  // Adapt for LG01-N (Ack not implemented yet, no CRC is sent for now) *******************
  char dataSend[100] = {0};
  char intStr[5];
  int bufIndex;

  for (bufIndex = 0; bufIndex < NODE_ID_LENGTH; bufIndex++) {
    dataSend[bufIndex] = sendBuf[bufIndex];
  }
  strcat(dataSend, "{\"number\":\"");
  for (int i = 0; i < DEVICE_ID_LENGTH; i++) {
    sprintf(intStr, "%02d", sendBuf[bufIndex++]);
    strcat(dataSend, intStr);
  }
  strcat(dataSend, "\",\"data\":{\"t0\":\"");
  sprintf(intStr, "%d", sendBuf[bufIndex++]);
  strcat(dataSend, intStr);
  strcat(dataSend, ".");
  sprintf(intStr, "%d", sendBuf[bufIndex++]);
  strcat(dataSend, intStr);
  strcat(dataSend, "\",\"t1\":\"");
  sprintf(intStr, "%d", sendBuf[bufIndex++]);
  strcat(dataSend, intStr);
  strcat(dataSend, ".");
  sprintf(intStr, "%d", sendBuf[bufIndex++]);
  strcat(dataSend, intStr);
  strcat(dataSend, "\",\"h\":\"");
  sprintf(intStr, "%d", sendBuf[bufIndex++]);
  strcat(dataSend, intStr);
  strcat(dataSend, "\",\"l\":\"");
  sprintf(intStr, "%d", 0); // LED is forced to 0 for now
  strcat(dataSend, intStr);
  strcat(dataSend, "\"}}");

  Serial.println(dataSend);
  int length = strlen(dataSend);
  Serial.print("Packet length: ");
  Serial.println(length);

  strcpy((char *)sendBuf, dataSend);
  rf95.send(sendBuf, strlen(dataSend)); //Send LoRa Data
  Serial.println("LoRa packet sent...");
  //***************************************************************************************

  /* No Ack yet for LG01-N */
  /*
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN]; //Reply data array
  uint8_t len = sizeof(buf);            //reply data length

  if (rf95.waitAvailableTimeout(3000)) // Check If there is reply in 3 seconds.
  {
    // Should be a reply message for us now
    if (rf95.recv(buf, &len)) //check if reply message is correct
    {
      if (buf[0] == node_id[0] && buf[1] == node_id[2] && buf[2] == node_id[2]) // Check if reply message has the our node ID
      {
        Serial.print("Got Reply from Gateway: "); //print reply
        // Convert node ID to text
        char str[4]; // Considered last 0 character for strings
        sprintf(str, "%d%d%d", buf[0], buf[1], buf[2]);
        buf[0] = str[0];
        buf[1] = str[1];
        buf[2] = str[2];
        Serial.println((char *)buf);

        Serial.print("RSSI: "); // Print RSSI
        Serial.println(rf95.lastRssi(), DEC);
      }
    }
    else
    {
      Serial.println("recv failed");
      //rf95.send(sendBuf, strlen((char *)sendBuf)); // Resend if no reply
    }
  }
  else
  {
    Serial.println("No reply, is LoRa gateway running?"); // No signal reply
    //rf95.send(sendBuf, strlen((char *)sendBuf));          // Resend data
  }
  Serial.println("");
  */

  // Power down mode
  delay(2000);  // Wait some time just in case to make everything stable
  rf95.sleep(); // Disable LoRa radio
  goToSleep(SLEEP_TIME);
}
