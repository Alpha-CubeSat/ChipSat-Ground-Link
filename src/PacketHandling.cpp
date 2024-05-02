#include "PacketHandling.h"

void handleTransmit()
{
    currentTime = millis();
    if (currentTime - transmittingStartTime > LISTEN_PERIOD)
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

    if (byteArr[0] == 'K') // Distinguish between callsign packets
    {
        enableInterrupt = true;
        state = radio.startReceive();
    }

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
        if (byteArr[17] << 5 == 1) // checks if the chipSat is in listen mode
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
    }
    else if (state == RADIOLIB_ERR_CRC_MISMATCH)
    {
        Serial.println(F("[SX1278] CRC error!"));
    }
    else
    {
        Serial.print(F("[SX1278] Failed, code "));
        Serial.println(state);
    }
}

void readSerial()
{
    static String inputBuffer = "";

    while (Serial.available() > 0)
    {
        char c = Serial.read();
        if (c == '\n')
        {
            if (inputBuffer.length() > 0)
            {
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
