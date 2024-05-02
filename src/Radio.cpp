#include "Radio.h"

void initSX1278()
{
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
    if (!enableInterrupt)
    {
        return;
    }
    receivedFlag = true;
}