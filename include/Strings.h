#pragma once
#include <Arduino.h>

const char read_txt[] PROGMEM = "Read";
const char writeOK_txt[] PROGMEM = "Write success!";
const char writeFailed_txt[] PROGMEM = "Write failed!";
const char readyToWrite_txt[] PROGMEM = "Ready to write";
const char sameKey_txt[] PROGMEM = "Same key!";
const char noKeys_txt[] PROGMEM = "No keys in EEPROM";

//-------------------- Macro ---------------------------
#define printString(str) \
for (byte i = 0; i < strlen_P(str); i++) { \
    display.print((char)pgm_read_byte(&str[i])); \
}
