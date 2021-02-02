#pragma once
#include <Arduino.h>

class KeysStorage {
    public:
    KeysStorage();
    uint8_t getKeyCount();
    uint8_t getKeyIndex();
    void incrementIndex();
    void decrementIndex();
    uint8_t getIndexKeyInROM(uint8_t *key);
    bool addKey(uint8_t *key);
    void getKeyByIndex(uint8_t *key);

    void clear();

    private:
    uint8_t _keyCountMax;
    uint8_t _keyCount;
    uint8_t _keyIndex;
    bool _isKeyEmpty(uint8_t *buffer);
};