#pragma once
#include <Arduino.h>

#define rfidBitRate 2       // bit rate rfid in kbps
#define RFID_PIN 11         // 

extern byte keyID[];        // key ID to write

bool searchRFID(bool copyKey);
byte write2rfid();
void sendEM_Marine(byte *buffer);
