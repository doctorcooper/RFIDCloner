#pragma once
#include <Arduino.h>

#define rfidBitRate 2       // bit rate rfid in kbps
#define RFID_PIN 11         // 

extern byte keyID[];        // key ID to write

bool searchRFID(bool copyKey);
bool write2rfid();
void SendEM_Marine(byte *buffer);
