/*
  Require Library: 
  https://github.com/dragino/RadioHead
  
  Upload Data to IoT Server ThingSpeak (https://thingspeak.com/):
  Support Devices: LoRa Shield + Arduino 
  
  Example sketch showing how to read Temperature from Dallas Temperature sensor DS18B20,  
  Then send the value to LoRa Gateway, the LoRa Gateway will send the value to the 
  IoT server
*/

#include <SPI.h>
#include <RH_RF95.h>

#include <OneWire.h>
#include <DallasTemperature.h>

RH_RF95 rf95;

// LoRa data configuration
byte bGlobalErr;
char dt_dat[5];              // Store Sensor Data (MAX 2 devices!)
char node_id[3] = {1, 1, 1}; // LoRa End Node ID
float frequency = 868.0;
unsigned int count = 1;

// Mesh status
#define MESH_PIN 3

// Data wire is plugged into port 2 on the Arduino

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

void initDT()
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

void initMeshStatus(void)
{
  pinMode(MESH_PIN, INPUT_PULLUP); // Internal pull up enabled
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

  for (int i = 0; i < 3; i++)
  {
    Serial.print(node_id[i], HEX);
  }
  Serial.println();
}

void setup()
{
  // Configure serial port
  Serial.begin(9600);

  // Temperature sensors init
  initDT();

  // Mesh status
  initMeshStatus();

  // LoRa Init
  initLoRa();
}

void readData()
{
  bGlobalErr = 0;

  // Get data from temperature sensors (up to 2 devices)
  sensors.requestTemperatures(); // Send the command to get temperature readings
  int i;
  for (i = 0; i < numberOfDevices; i++)
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

  // Get mesh status
  dt_dat[i * 2] = digitalRead(MESH_PIN);
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
  int i; // Counter

  Serial.print("###########    ");
  Serial.print("COUNT=");
  Serial.print(count);
  Serial.println("    ###########");
  count++;
  readData();
  char data[50] = {0};
  int dataLength = 3 + numberOfDevices * 2 + 1; // Payload Length: node_id + temperature(s) + mesh
  // Use data[0], data[1], data[2] as Node ID
  data[0] = node_id[0];
  data[1] = node_id[1];
  data[2] = node_id[2];
  // Get temperature(s)
  for (i = 0; i < numberOfDevices; i++)
  {
    data[3 + (i * 2)] = dt_dat[i * 2];       //Get Temperature Integer Part
    data[4 + (i * 2)] = dt_dat[(i * 2) + 1]; //Get Temperature Decimal Part
  }
  // Get mesh status
  data[3 + (i * 2)] = dt_dat[i * 2]; // Mesh status
  switch (bGlobalErr)
  {
  case 0:
    for (i = 0; i < numberOfDevices; i++)
    {
      Serial.print("Temperature ");
      Serial.print(i);
      Serial.print(": ");
      Serial.print(data[3 + (i * 2)], DEC); //Show temperature
      Serial.print(",");
      Serial.print(data[4 + (i * 2)], DEC); //Show temperature
      Serial.println("C");
    }
    Serial.print("Mesh status: ");
    Serial.println(data[3 + (i * 2)], DEC); // Show mesh status
    break;
  default:
    Serial.println("Error: Unrecognized code encountered.");
    break;
  }

  uint16_t crcData = CRC16((unsigned char *)data, dataLength); //get CRC DATA
  //Serial.println(crcData,HEX);

  Serial.print("Data to be sent(without CRC): ");

  for (i = 0; i < dataLength; i++)
  {
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  unsigned char sendBuf[50] = {0};

  for (i = 0; i < dataLength; i++)
  {
    sendBuf[i] = data[i];
  }

  sendBuf[dataLength] = (unsigned char)crcData;            // Add CRC to LoRa Data
  sendBuf[dataLength + 1] = (unsigned char)(crcData >> 8); // Add CRC to LoRa Data

  Serial.print("Data to be sent(with CRC):    ");
  for (i = 0; i < (dataLength + 2); i++)
  {
    Serial.print(sendBuf[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  rf95.send(sendBuf, dataLength + 2); //Send LoRa Data

  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN]; //Reply data array
  uint8_t len = sizeof(buf);            //reply data length

  if (rf95.waitAvailableTimeout(3000)) // Check If there is reply in 3 seconds.
  {
    // Should be a reply message for us now
    if (rf95.recv(buf, &len)) //check if reply message is correct
    {
      if (buf[0] == node_id[0] && buf[1] == node_id[2] && buf[2] == node_id[2]) // Check if reply message has the our node ID
      {
        pinMode(4, OUTPUT);
        digitalWrite(4, HIGH);
        Serial.print("Got Reply from Gateway: "); //print reply
        // Convert node ID to text
        char str[3];
        sprintf(str, "%d%d%d", buf[0], buf[1], buf[2]);
        buf[0] = str[0];
        buf[1] = str[1];
        buf[2] = str[2];
        Serial.println((char *)buf);

        delay(400);
        digitalWrite(4, LOW);
        //Serial.print("RSSI: ");  // print RSSI
        //Serial.println(rf95.lastRssi(), DEC);
      }
    }
    else
    {
      Serial.println("recv failed");
      rf95.send(sendBuf, strlen((char *)sendBuf)); //resend if no reply
    }
  }
  else
  {
    Serial.println("No reply, is LoRa gateway running?"); //No signal reply
    rf95.send(sendBuf, strlen((char *)sendBuf));          //resend data
  }
  delay(10000); // Send sensor data every 10 seconds
  Serial.println("");
}
