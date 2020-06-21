#pragma once
#include <Arduino.h>

#define rfidBitRate 2       // Скорость обмена с rfid в kbps
#define FrequencyGen 11

// Check on even cols with data
bool vertEvenCheck(byte* buf);
byte ttAComp(unsigned long timeOut);
bool readEM_Marie(byte* buf);
// void rfidACsetOn();
bool searchEM_Marine(bool copyKey);
