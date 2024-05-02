#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "utilities.h"

#ifdef HAS_DISPLAY
#include <U8g2lib.h>
#endif

#ifndef BOARDS_H
#define BOARDS_H

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C *u8g2;
extern SPIClass SDSPI;

void initBoard();

#endif
