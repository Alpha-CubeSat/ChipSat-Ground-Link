/*
   This script listens and sends LoRa transmissions using SX1278 LoRa modules to ChipSats
   for the Alpha CubeSat mission.

   To change PacketType to NO_OP or CHANGE_DOWNLINK_PERIOD uplinks write to the Serial Monitor
   with:
    [N] for NO_OP
    [C] for CHANGE_DOWNLINK_PERIOD. After writing C, enter the desired downlink period (units TBD)

   To successfully receive data, the following settings have to be the same
   on both transmitter and receiver:
    - carrier frequency
    - bandwidth
    - spreading factor
    - coding rate
    - sync word
    - preamble length

   References:
    For default module settings, see the wiki page
    https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx127xrfm9x---lora-modem

    For full API reference, see the GitHub Pages
    https://jgromes.github.io/RadioLib/
*/

#include <RadioLib.h>
#include "constants.h"
#include "boards.h"
#include "utilities.h"
#include "packet_type_enums.h"
#include "module_state_enums.h"

SX1278 radio = new Module(RADIO_CS_PIN, RADIO_DI0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

PacketType currentPacketType = NO_OP;
ModuleState currentState = LISTENING;
volatile bool receivedFlag = false;
volatile bool enableInterrupt = true;

uint16_t newDownlinkPeriod;

unsigned long transmittingStartTime = 0;
unsigned long currentTime = 0;

void setup()
{
  initBoard();
  delay(1500);
  initSX1278();
}

void loop()
{
  if (currentState == TRANSMITTING)
  {
    handleTransmit();
  }
  else if (currentState == LISTENING && receivedFlag)
  {
    handleReceive();
  }
  else if (Serial.available() > 0)
  {
    readSerial();
  }
}

void initSX1278()
{
  // initialize SX1278 with specified settings
  Serial.print(F("[SX1278] Initializing ... "));
  int state = radio.begin(FREQ, BW, SF, CR, SW, PWR, PL, GN);

  Serial.print(F("Radio: Setting frequency ... "));
  state = radio.setFrequency(FREQ);
  if (state == 0)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.println("Frequency initialization error");
  }

  Serial.print(F("Radio: Setting Output Power parameter ... "));
  state = radio.setOutputPower(PWR);
  if (state == 0)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("Output Power initialization error"));
  }

  Serial.print(F("Radio: Setting Spreading Factor parameter ... "));
  state = radio.setSpreadingFactor(SF);
  if (state == 0)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("Spreading Factor initialization error"));
  }

  // set CRC parameter to true so it matches the CRC parameter on the TinyGS side
  Serial.print(F("[RF96] Setting CRC parameter ... "));
  state = radio.setCRC(true);
  if (state == 0)
  { // ERR_NONE
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("CRC initialization error"));
    Serial.println(state);
    while (true)
      ;
  }

  // set forceLDRO parameter to true so it matches the forceLDRO parameter on the TinyGS side
  Serial.print(F("[RF96] Setting forceLDRO parameter ... "));
  state = radio.forceLDRO(true); // FLDRO = 1 on TinyGS side
  if (state == 0)
  { // ERR_NONE
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("forceLDRO initialization error"));
    Serial.println(state);
    while (true)
      ;
  }

#ifdef HAS_DISPLAY
  if (u8g2)
  {
    if (state != RADIOLIB_ERR_NONE)
    {
      u8g2->clearBuffer();
      u8g2->drawStr(0, 12, "Initializing: FAIL!");
      u8g2->sendBuffer();
    }
  }
#endif

  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
      ;
  }

  radio.setDio0Action(setFlag);
  Serial.print(F("[SX1278] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("SX1278 initialization ... success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
      ;
  }
}

void setFlag(void)
{
  // check if the interrupt is enabled
  if (!enableInterrupt)
  {
    return;
  }
  // we got a packet, set the flag
  receivedFlag = true;
}

void handleTransmit()
{
  currentTime = millis();
  if (currentTime - transmittingStartTime > 30000)
  {
    radio.startReceive();
    currentState = LISTENING;
    enableInterrupt = true;
  }
  else
  {
    sendResponse();
  }
}

void handleReceive()
{
  enableInterrupt = false;
  receivedFlag = false;

  byte byteArr[15];
  int state = radio.readData(byteArr, 15);

  Serial.println("Downlinked Report:");
  int i;
  for (i = 0; i < sizeof(byteArr); i++)
  {
    Serial.println(byteArr[i]);
  }

  Serial.println("");

  // packet was successfully received
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("[SX1278] Received packet!"));
    if (byteArr[14] == 1) // checks if the chipSat is in listen mode
    {
      currentState = TRANSMITTING;
      transmittingStartTime = millis();
    }
    else
    {
      Serial.print(F("[SX1278] Going back to listening ... "));
      enableInterrupt = true;
      state = radio.startReceive();
      if (state == RADIOLIB_ERR_NONE)
      {
        Serial.println(F("Listening again."));
      }
      else
      {
        Serial.print(F("Failed to go back to listening, code "));
        Serial.println(state);
      }
    }
#ifdef HAS_DISPLAY
    if (u8g2)
    {
      u8g2->clearBuffer();
      char buf[256];
      u8g2->drawStr(0, 12, "Received OK!");
      snprintf(buf, sizeof(buf), "RSSI: %.2f", radio.getRSSI());
      u8g2->drawStr(0, 45, buf);
      snprintf(buf, sizeof(buf), "SNR: %.2f", radio.getSNR());
      u8g2->drawStr(0, 60, buf);
      u8g2->sendBuffer();
    }
#endif
  }
  else if (state == RADIOLIB_ERR_CRC_MISMATCH)
  {
    // packet was received, but is malformed
    Serial.println(F("[SX1278] CRC error!"));
  }
  else
  {
    // some other error occurred
    Serial.print(F("[SX1278] Failed, code "));
    Serial.println(state);
  }
}

void readSerial()
{
  static String inputBuffer = ""; // Buffer to hold the input string from serial

  // Check if there's data to read
  while (Serial.available() > 0)
  {
    char c = Serial.read(); // Read the next character
    if (c == '\n')
    {
      // Newline character received, process the command
      if (inputBuffer.length() > 0)
      {
        // Check the first character to determine the command
        char command = inputBuffer.charAt(0);

        if (command == 'N')
        {
          Serial.println("NO_OP SELECTED");
          currentPacketType = NO_OP;
        }
        else if (command == 'C' && inputBuffer.length() > 1)
        {
          // Assume the rest of the buffer contains the downlink period value
          String downlinkPeriodStr = inputBuffer.substring(1); // Get the substring after 'C'
          newDownlinkPeriod = downlinkPeriodStr.toInt();
          Serial.print("newDownlinkPeriod: ");
          Serial.println(newDownlinkPeriod);
          currentPacketType = CHANGE_DOWNLINK_PERIOD;
        }
        else
        {
          Serial.println("ERROR: Unknown command or missing value.");
        }
      }
      inputBuffer = "";
    }
    else if (c != '\r')
    {
      inputBuffer += c;
    }
  }
}

void sendResponse()
{
  byte packet[3] = {0x00, 0x00, 0x00};

  switch (currentPacketType)
  {
  case NO_OP:
    break;
  case CHANGE_DOWNLINK_PERIOD:
    packet[0] = 0x11;
    packet[1] = (byte)(newDownlinkPeriod >> 8);
    packet[2] = (byte)(newDownlinkPeriod & 0xFF);
    break;
  default:
    Serial.println("Unknown command. Check serial input.");
    break;
  }

  radio.transmit(packet, 3);
}
