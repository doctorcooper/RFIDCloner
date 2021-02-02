#include "KeysStorage.h"
#include <EEPROM.h>

KeysStorage::KeysStorage() {
    _keyCount = EEPROM[0];                                  // Get count of keys 
    _keyCountMax = EEPROM.length() / 5 - 1;                 // Get maximun count of keys
    if (_keyCountMax > 20) { _keyCountMax = 20; }           // It doesn't be over 20
    if (_keyCount > _keyCountMax) { _keyCount = 0; }
    if (_keyCount != 0) { _keyIndex = EEPROM[1]; }
}

uint8_t KeysStorage::getKeyCount() { return _keyCount; }
uint8_t KeysStorage::getKeyIndex() { return _keyIndex; }

void KeysStorage::incrementIndex() {
    _keyIndex++;
    if (_keyIndex > _keyCount) { _keyIndex = 1; }
    EEPROM.update(1, _keyIndex);
}

void KeysStorage::decrementIndex() {
    _keyIndex--;
    if (_keyIndex < 1) { _keyIndex = _keyCount; }
    EEPROM.update(1, _keyIndex);
}

bool KeysStorage::_isKeyEmpty(uint8_t *buffer) {             // if key equal zeros -> true
    uint8_t result = 0;
    for (uint8_t i = 0; i < 5; i++) {
        result += buffer[i];
    }
    return (result == 0);
}

uint8_t KeysStorage::getIndexKeyInROM(uint8_t *key) {       // возвращает индекс или ноль если нет в ROM 
    uint8_t buffer[5]; 
    bool eq = true;

    for (byte j = 1; j <= _keyCount; j++) {                 // ищем ключ в eeprom. 
        EEPROM.get(j * sizeof(buffer), buffer);
        for (byte i = 0; i < 5; i++) {
            if (buffer[i] != key[i]) { 
                eq = false;
                break; 
            }
        }
        if (eq) return j;
        eq = true;
    }
    return 0;
}

bool KeysStorage::addKey(uint8_t *key) {

    if (_isKeyEmpty(key)) { return false; }

    uint8_t buffer1[5]; 
    uint8_t index = getIndexKeyInROM(key);
              
    if (index != 0) { // ищем ключ в eeprom. Если находим, то не делаем запись, а индекс переводим в него 
        _keyIndex = index;
        EEPROM.update(1, _keyIndex);
        return false; 
    }

    if (_keyCount <= _keyCountMax) { _keyCount++; }
    if (_keyCount < _keyCountMax) { _keyIndex = _keyCount; } else { _keyIndex++; }
    if (_keyIndex > _keyCount) { _keyIndex = 1; }

    for (byte i = 0; i < 5; i++) { buffer1[i] = key[i]; }

    EEPROM.put(_keyIndex * sizeof(buffer1), buffer1);
    EEPROM.update(0, _keyCount);
    EEPROM.update(1, _keyIndex);
    return true;
}

void KeysStorage::getKeyByIndex(uint8_t *key) {
    uint8_t buffer1[5];
    uint8_t address = _keyIndex * sizeof(buffer1);
    if (address > EEPROM.length()) { return; }
    EEPROM.get(address, buffer1);
    for (byte i = 0; i < 5; i++) { 
        key[i] = buffer1[i]; 
    }
}

void KeysStorage::clear() {
    EEPROM.update(0, 0); 
    EEPROM.update(1, 0);
    _keyCount = 0; 
    _keyIndex = 0;
}
