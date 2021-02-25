#include "RFID-TAG-125kHz.h"
#include <Arduino.h>


// Constructor (clock divider(1 == 16MHz, 2 == 8MHz))
RFID_TAG::RFID_TAG(uint8_t divider) {
    _pwmDivider = divider;
}

// Working with data
uint8_t RFID_TAG::checkParity(uint8_t byte) {
    uint8_t par = 0;
    for (uint8_t i = 0; i < 8; i++) {
        par ^= ((byte >> i) & 1);
    }
    return par;
}

void RFID_TAG::getUID(uint8_t *rawData, uint8_t *uid) {
    uid[0] = (0b01111000 & rawData[1]) << 1 | (0b11 & rawData[1]) << 2 | rawData[2] >> 6;
    uid[1] = (0b00011110 & rawData[2]) << 3 | rawData[3] >> 4;
    uid[2] = rawData[3] << 5 | (0b10000000 & rawData[4]) >> 3 | (0b00111100 & rawData[4]) >> 2;
    uid[3] = rawData[4] << 7 | (0b11100000 & rawData[5]) >> 1 | (0b1111 & rawData[5]);
    uid[4] = (0b01111000 & rawData[6]) << 1 | (0b11 & rawData[6]) << 2 | rawData[7] >> 6;
}

void RFID_TAG::getUID(uint8_t *uid) {
    for (uint8_t i = 0; i < 5; i++) {
        uid[i] = _uid[i];
    }    
}

void RFID_TAG::getRawData(uint8_t *uid, uint8_t *rawData) {
    rawData[0] = 0b11111111;
    rawData[1] = 0b10000000 | (uid[0] & 0b11110000) >> 1 | checkParity((uid[0] & 0b11110000)) << 2 | (uid[0] & 0b00001100) >> 2;
    rawData[2] = (uid[0] & 0b00000011) << 6 | checkParity(uid[0] & 0b00001111) << 5 | (uid[1] & 0b11110000) >> 3 | checkParity(uid[1] & 0b11110000);
    rawData[3] = (uid[1] & 0b00001111) << 4 | checkParity(uid[1] & 0b00001111) << 3 | (uid[2] & 0b11100000) >> 5;
    rawData[4] = (uid[2] & 0b00010000) << 3 | checkParity(uid[2] & 0b11110000) << 6 | (uid[2] & 0b00001111) << 2 | checkParity(uid[2] & 0b00001111) << 1 | (uid[3] & 0b10000000) >> 7;
    rawData[5] = (uid[3] & 0b01110000) << 1 | checkParity(uid[3] & 0b11110000) << 4 | (uid[3] & 0b00001111);
    rawData[6] = checkParity(uid[3] & 0b00001111) << 7 | (uid[4] & 0b11110000) >> 1 | checkParity(uid[4] & 0b11110000) << 2 | (uid[4] & 0b00001100) >> 2;
    rawData[7] = (uid[4] & 0b00000011) << 6 | checkParity(uid[4] & 0b00001111) << 5;

    uint8_t col = 0;
    /*
    This is not work
    for (uint8_t i = 0; i < 4; i++) {
        col = 0;
        for (uint8_t j = 0; j < 5; j++) {
            col ^= (uid[j] & (0b10000000 >> i) >> (7 - i)) ^ (uid[j] & (0b00001000 >> i) >> (3 - i));
        }
        rawData[7] |= col << (4 - i);
    }
    */
    col = (uid[0] & 0b10000000) << 0 | (uid[0] & 0b00001000) << 3 | (uid[1] & 0b10000000) >> 2 | (uid[1] & 0b00001000) << 1 | (uid[2] & 0b10000000) >> 4 | (uid[2] & 0b00001000) >> 1 | (uid[3] & 0b10000000) >> 6 | (uid[3] & 0b00001000) >> 3;
    col = checkParity(col) << 7;
    col |= (uid[4] & 0b10000000) >> 6 | (uid[4] & 0b00001000) >> 3;
    col = checkParity(col);
    rawData[7] |= col << 4;

    col = 0;
    col = (uid[0] & 0b01000000) << 1 | (uid[0] & 0b00000100) << 4 | (uid[1] & 0b01000000) >> 1 | (uid[1] & 0b00000100) << 2 | (uid[2] & 0b01000000) >> 3 | (uid[2] & 0b00000100) >> 0 | (uid[3] & 0b01000000) >> 5 | (uid[3] & 0b00000100) >> 2;
    col = checkParity(col) << 7;
    col |= (uid[4] & 0b01000000) >> 5 | (uid[4] & 0b00000100) >> 2;
    col = checkParity(col);
    rawData[7] |= col << 3;

    col = 0;
    col = (uid[0] & 0b00100000) << 2 | (uid[0] & 0b00000010) << 5 | (uid[1] & 0b00100000) >> 0 | (uid[1] & 0b00000010) << 3 | (uid[2] & 0b00100000) >> 2 | (uid[2] & 0b00000010) << 1 | (uid[3] & 0b00100000) >> 4 | (uid[3] & 0b00000010) >> 1;
    col = checkParity(col) << 7;
    col |= (uid[4] & 0b00100000) >> 4 | (uid[4] & 0b00000010) >> 1;
    col = checkParity(col);
    rawData[7] |= col << 2;

    col = 0;
    col = (uid[0] & 0b00010000) << 3 | (uid[0] & 0b00000001) << 6 | (uid[1] & 0b00010000) << 1 | (uid[1] & 0b00000001) << 4 | (uid[2] & 0b00010000) >> 1 | (uid[2] & 0b00000001) << 2 | (uid[3] & 0b00010000) >> 3 | (uid[3] & 0b00000001) >> 0;
    col = checkParity(col) << 7;
    col |= (uid[4] & 0b00010000) >> 3 | (uid[4] & 0b00000001) >> 0;
    col = checkParity(col);
    rawData[7] |= col << 1;
}

bool RFID_TAG::columnParityCheck(uint8_t *buffer) {                  // Check on even cols with data
    uint8_t k;
    k = (1 & buffer[1] >> 6) + (1 & buffer[1] >> 1) + (1 & buffer[2] >> 4) + (1 & buffer[3] >> 7) + (1 & buffer[3] >> 2) + (1 & buffer[4] >> 5) + (1 & buffer[4]) + (1 & buffer[5] >> 3) + (1 & buffer[6] >> 6) + (1 & buffer[6] >> 1) + (1 & buffer[7] >> 4);
    if (k & 1)
        return false;
    k = (1 & buffer[1] >> 5) + (1 & buffer[1]) + (1 & buffer[2] >> 3) + (1 & buffer[3] >> 6) + (1 & buffer[3] >> 1) + (1 & buffer[4] >> 4) + (1 & buffer[5] >> 7) + (1 & buffer[5] >> 2) + (1 & buffer[6] >> 5) + (1 & buffer[6]) + (1 & buffer[7] >> 3);
    if (k & 1)
        return false;
    k = (1 & buffer[1] >> 4) + (1 & buffer[2] >> 7) + (1 & buffer[2] >> 2) + (1 & buffer[3] >> 5) + (1 & buffer[3]) + (1 & buffer[4] >> 3) + (1 & buffer[5] >> 6) + (1 & buffer[5] >> 1) + (1 & buffer[6] >> 4) + (1 & buffer[7] >> 7) + (1 & buffer[7] >> 2);
    if (k & 1)
        return false;
    k = (1 & buffer[1] >> 3) + (1 & buffer[2] >> 6) + (1 & buffer[2] >> 1) + (1 & buffer[3] >> 4) + (1 & buffer[4] >> 7) + (1 & buffer[4] >> 2) + (1 & buffer[5] >> 5) + (1 & buffer[5]) + (1 & buffer[6] >> 3) + (1 & buffer[7] >> 6) + (1 & buffer[7] >> 1);
    if (k & 1)
        return false;
    if (1 & buffer[7])
        return false;
    return true;
}

// Work with hardware
// Reading
uint8_t RFID_TAG::ttAComp(uint32_t timeOut) {                       // pulse 0 or 1 or -1 if timeout 
    uint8_t AcompState, AcompInitState;
    uint32_t tEnd = micros() + timeOut;
    AcompInitState = bitRead(ACSR, ACO);                            // читаем флаг компаратора
    do {
        AcompState = bitRead(ACSR, ACO);                            // читаем флаг компаратора
        if (AcompState != AcompInitState) {
            delayMicroseconds(125);                                 // 1/4 Period on 2 kBps = 125 mks
            AcompState = bitRead(ACSR, ACO);                        // читаем флаг компаратора
            delayMicroseconds(250);                                 // 1/2 Period on 2 kBps = 250 mks
            return AcompState;
        }
    } while (micros() < tEnd);
    return 2;                                                       // таймаут, компаратор не сменил состояние
}

void RFID_TAG::setAC(bool state) {                                  // включаем генератор 125кГц
    if (state) {
        pinMode(GENERATOR_PIN, OUTPUT);
        TCCR2A = bit(COM2A0) | bit(COM2B1) | bit(WGM21) | bit(WGM20); // Включаем режим Toggle on Compare Match на COM2A (pin 11) и счет таймера2 до OCR2A
        TCCR2B = bit(WGM22) | bit(CS20);                            // Задаем делитель для таймера2 = 1 (16 мГц)
        OCR2A = 63 / _pwmDivider;                                   // 63 тактов на период. Частота на COM2A (pin 11) 16000/64/2 = 125 кГц, Скважнось COM2A в этом режиме всегда 50%
        OCR2B = 31 / _pwmDivider;                                   // Скважность COM2B 32/64 = 50%  Частота на COM2A (pin 3) 16000/64 = 250 кГц
        bitClear(ADCSRB, ACME);                                     // отключаем мультиплексор AC
        bitClear(ACSR, ACBG);                                       // отключаем от входа Ain0 1.1V
    } else {
        TCCR2A &= 0b00111111;
    }
}

bool RFID_TAG::readRFIDTag(uint8_t *buffer) {
    uint32_t tEnd = millis() + 50;
    uint8_t ti, j = 0, k = 0;
    for (uint8_t i = 0; i < 64; i++) {                          // читаем 64 bit
        ti = ttAComp();
        if (ti == 2) { break; }                                 // timeout                                            
        if ((ti == 0) && (i < 9)) {                             // если не находим 9 стартовых единиц - начинаем сначала
            if (millis() > tEnd) {
                ti = 2;
                break;                                          // timeout
            } 
            i = -1;
            j = 0;
            continue;
        }
        if ((i > 8) && (i < 59)) {                              // начиная с 9-го бита проверяем контроль четности каждой строки 
            if (ti) { k++; }                                    // считаем кол-во единиц
            if ((i - 9) % 5 == 4) {                             // конец строки с данными из 5-и бит,
                if (k & 1) {                                    // если нечетно - начинаем сначала
                    i = -1;
                    j = 0;
                    k = 0;
                    continue;
                } else { k = 0; }
            }
        }
        if (ti) { 
            bitSet(buffer[i >> 3], 7 - j);
        } else {
            bitClear(buffer[i >> 3], 7 - j);
        }
        j++;
        if (j > 7) { j = 0; }
    }
    if (ti == 2) { 
        return false;                                           // timeout
    }                           
    return columnParityCheck(buffer);
}

bool RFID_TAG::searchTag(bool copyKey) {
    readOperationIsBusy = false;
    bool result = false;
    setAC(true);                                                // включаем генератор 125кГц и компаратор
    delay(6);                                                   // переходные процессы детектора
    if (readRFIDTag(_keyBuffer)) {
        debugPrintArray(_keyBuffer);
        if (copyKey) { getUID(_keyBuffer, _uid); }
        setAC(false); 
        readOperationIsBusy = true;
        result = true;
    } else {
        setAC(false);                                           // Оключить ШИМ COM2A (pin 11)
        readOperationIsBusy = true;
        result = false;
    }
    return result;
}

// Writing
void RFID_TAG::rfidGap(uint16_t timeout) {                      //  Restart PWM on COM2A (pin 11)
    setAC(false); 
    delayMicroseconds(timeout);
    bitSet(TCCR2A, COM2A0);
}

void RFID_TAG::txBitRfid(uint8_t data) {
    if (data & 1) {
        delayMicroseconds(54 * 8);
    } else {
        delayMicroseconds(24 * 8);
    }
    rfidGap(19 * 8);
}

bool RFID_TAG::sendOpT5557(uint8_t opCode, uint32_t password = 0, uint8_t lockBit = 0, uint32_t data = 0, uint8_t blokAddr = 1) {
    txBitRfid(opCode >> 1);
    txBitRfid(opCode & 1);
    if (opCode == 0b00) { return true; }
    txBitRfid(lockBit & 1);
    if (data != 0) {
        for (uint8_t i = 0; i < 32; i++) {
            txBitRfid((data >> (31 - i)) & 1);
        }
    }
    txBitRfid(blokAddr >> 2);
    txBitRfid(blokAddr >> 1);
    txBitRfid(blokAddr & 1); 
    delay(4);               
    return true;
}

bool RFID_TAG::T5557_blockRead(uint8_t* buffer) {
    uint8_t ti, j = 0; 
    for (uint8_t i = 0; i < 33; i++) {                                                             
        ti = ttAComp();                                     // читаем стартовый 0 и 32 значащих bit
        if (ti == 2) { break; }                                 // timeout
        if ((ti == 1) && (i == 0)) {                            // если не находим стартовый 0 - это ошибка
            ti = 2; 
            break; 
        }     
        if (i > 0) {                                            // начиная с 1-го бита пишем в буфер
            if (ti) {
                bitSet(buffer[(i - 1) >> 3], 7 - j);
            } else {
                bitClear(buffer[(i - 1) >> 3], 7 - j);
            }
            j++; 
            if (j > 7) { j = 0; }
        }
    }
    if (ti == 2) return false;                                  // timeout
    return true;
}

bool RFID_TAG::checkWriteState() {
    uint32_t data32, data33; 
    uint8_t buffer[4] = {0, 0, 0, 0}; 
    setAC(true);                                                // включаем генератор 125кГц и компаратор
    delay(13);                                                  // 13 мс длятся переходные процессы детектора
    rfidGap(30 * 8);                                            // start gap
    sendOpT5557(0b11, 0, 0, 0, 1);                              // переходим в режим чтения Vendor ID 
    if (!T5557_blockRead(buffer)) { return false; } 
    data32 = (uint32_t)buffer[0] << 24 | (uint32_t)buffer[1] << 16 | (uint32_t)buffer[2] << 8 | (uint32_t)buffer[3];
    delay(4);
    rfidGap(20 * 8);                                        
    data33 = 0b00000000000101001000000001000000 | (0 << 4);     // конфиг регистр 0b00000000000101001000000001000000
    sendOpT5557(0b10, 0, 0, data33, 0);                         // передаем конфиг регистр
    delay(4);
    rfidGap(30 * 8);                                            // start gap
    sendOpT5557(0b11, 0, 0, 0, 1);                              // переходим в режим чтения Vendor ID 
    if (!T5557_blockRead(buffer)) { return false; } 
    data33 = (uint32_t)buffer[0] << 24 | (uint32_t)buffer[1] << 16 | (uint32_t)buffer[2] << 8 | (uint32_t)buffer[3];
    sendOpT5557(0b00, 0, 0, 0, 0);                              // send Reset
    delay(6);
    if (data32 != data33) { return false; }  
    return true;
}

bool RFID_TAG::write2rfidT5557(uint8_t *buffer) {
    bool result;
    uint32_t data32;
    delay(6);
    for (byte k = 0; k < 2; k++) { // send key data
        data32 = (uint32_t)buffer[0 + (k << 2)] << 24 | (uint32_t)buffer[1 + (k << 2)] << 16 | (uint32_t)buffer[2 + (k << 2)] << 8 | (uint32_t)buffer[3 + (k << 2)];
        rfidGap(30 * 8);                                        // start gap
        sendOpT5557(0b10, 0, 0, data32, k + 1);                 // передаем 32 бита ключа в block k
        delay(6);
    }
    delay(6);
    rfidGap(30 * 8);                                            // start gap
    sendOpT5557(0b00);
    delay(4);
    result = readRFIDTag(_keyBuffer);
    setAC(false);                                               // Оключить ШИМ COM2A (pin 11)
    for (byte i = 0; i < 8; i++) {
        if (_keyBuffer[i] != buffer[i]) {
            return false;
        }
    }
    return result;
}

uint8_t RFID_TAG::writeTag(uint8_t *uid) {
    writeOperationIsBusy = false;
    bool check = true;
    uint8_t rawKey[8];
    getRawData(uid, rawKey);
    if (searchTag(false)) {
        debugPrintArray(_keyBuffer);
        debugPrintArray(rawKey);
        for (byte i = 0; i < 8; i++) {
            if (_keyBuffer[i] != rawKey[i]) {  
                check = false;                                  // сравниваем код для записи с тем, что уже записано в ключе.                                                                                         
                break;                                          // если коды совпадают, ничего писать не нужно
            }   
        }                                                                        
        if (check) {
            writeOperationIsBusy = true;
            return 3;                                           // Error - same key
        }                                                                   
    }
    
    if (checkWriteState()) {
        if (write2rfidT5557(rawKey)) { 
            writeOperationIsBusy = true;
            return 1;                                           // Write ok
        } else { 
            writeOperationIsBusy = true;
            return 2;                                           // Write error
        }                                                                    
    } else {
        writeOperationIsBusy = true;
        return 0;                                               // Idle
    }                                                     
}

// Emulation
void RFID_TAG::emulateKey(uint8_t *keyUID) {                    // send key (Test version)
    getRawData(keyUID, _keyBuffer);
}

void RFID_TAG::prepareEmulator() {
    setAC(false);    
    digitalWrite(GENERATOR_PIN, LOW);
    delay(20);
}

void RFID_TAG::debugPrintArray(uint8_t* array) {
    #ifdef DEBUG
    for (uint8_t i = 0; i < 8; i++) {
        Serial.print(array[i], BIN);
        Serial.print(" ");
    }
    Serial.println();
    #endif
}

uint8_t* RFID_TAG::getRawKey() {
    return _keyBuffer;
}

