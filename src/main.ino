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

#include "PacketHandling.h"

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