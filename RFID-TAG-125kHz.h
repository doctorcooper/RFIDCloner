#pragma once
#include <Arduino.h>
/*
    Works on Timer2(Pin 11), Analog comp(Pin 6, 7)
*/

class RFID_TAG {
public:
    RFID_TAG(uint8_t divider);                       // Constructor (clock divider(1 == 16MHz, 2 == 8MHz))
    bool searchTag(bool copyKey);                       // Action for reading tag
    uint8_t writeTag(uint8_t *uid);
    void getUID(uint8_t *keyUID);                       // Get UID Tag
    void emulateKey(uint8_t *keyUID);
private:
    uint8_t checkParity(uint8_t byte);                  // Get parity bit
    void getUID(uint8_t *rawData, uint8_t *uid);        // Get UID Tag from raw data
    void getRawData(uint8_t *uid, uint8_t *rawData);    // Get Raw data from UID
    bool columnParityCheck(uint8_t *buffer);            // Check parity bit from column of data
    uint8_t ttAComp(uint32_t timeOut);                  // Read data from Analog Comparer
    bool readRFIDTag(uint8_t *buffer);                  // Read data from tag
    void setAC(bool state);                             // Control Analog Comparer //ac set on or TCCR...

    void rfidGap(uint16_t timeout);                     // Restart PWM
    void txBitRfid(uint8_t data);                       // 
    bool sendOpT5557(uint8_t opCode, uint32_t password, 
                     uint8_t lockBit, uint32_t data, 
                     uint8_t blokAddr);
    bool T5557_blockRead(uint8_t* buffer);
    bool checkWriteState();
    bool write2rfidT5557(uint8_t *buffer);
    
    const uint8_t GENERATOR_PIN = 11;                   // Emulator PIN

    uint8_t _uid[5];                                    // UID
    uint8_t _keyBuffer[8];                              // Buffer for raw data

    uint8_t _pwmDivider;                             // Multiplier for PWM (1 == 16MHz, 2 == 8MHz)
};