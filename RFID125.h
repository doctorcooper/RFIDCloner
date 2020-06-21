#pragma once
#include <Arduino.h>

#define rfidBitRate 2       // bit rate rfid in kbps
#define FrequencyGen 11     // 

extern byte keyID[];        // key ID to write

bool searchEM_Marine(bool copyKey);
bool write2rfid();
