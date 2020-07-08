#include "RFID125.h"
#include <Arduino.h>

byte rfidData[5];                                               // Number on the key
byte keyID[8];                                                  // key id for writing
byte addr[8];                                                   // temp buffer

bool vertEvenCheck(byte *buffer)                                // Check on even cols with data
{
    byte k;
    k = 1 & buffer[1] >> 6 + 1 & buffer[1] >> 1 + 1 & buffer[2] >> 4 + 1 & buffer[3] >> 7 + 1 & buffer[3] >> 2 + 1 & buffer[4] >> 5 + 1 & buffer[4] + 1 & buffer[5] >> 3 + 1 & buffer[6] >> 6 + 1 & buffer[6] >> 1 + 1 & buffer[7] >> 4;
    if (k & 1)
        return false;
    k = 1 & buffer[1] >> 5 + 1 & buffer[1] + 1 & buffer[2] >> 3 + 1 & buffer[3] >> 6 + 1 & buffer[3] >> 1 + 1 & buffer[4] >> 4 + 1 & buffer[5] >> 7 + 1 & buffer[5] >> 2 + 1 & buffer[6] >> 5 + 1 & buffer[6] + 1 & buffer[7] >> 3;
    if (k & 1)
        return false;
    k = 1 & buffer[1] >> 4 + 1 & buffer[2] >> 7 + 1 & buffer[2] >> 2 + 1 & buffer[3] >> 5 + 1 & buffer[3] + 1 & buffer[4] >> 3 + 1 & buffer[5] >> 6 + 1 & buffer[5] >> 1 + 1 & buffer[6] >> 4 + 1 & buffer[7] >> 7 + 1 & buffer[7] >> 2;
    if (k & 1)
        return false;
    k = 1 & buffer[1] >> 3 + 1 & buffer[2] >> 6 + 1 & buffer[2] >> 1 + 1 & buffer[3] >> 4 + 1 & buffer[4] >> 7 + 1 & buffer[4] >> 2 + 1 & buffer[5] >> 5 + 1 & buffer[5] + 1 & buffer[6] >> 3 + 1 & buffer[7] >> 6 + 1 & buffer[7] >> 1;
    if (k & 1)
        return false;
    if (1 & buffer[7])
        return false;
    rfidData[0] = (0b01111000 & buffer[1]) << 1 | (0b11 & buffer[1]) << 2 | buffer[2] >> 6;
    rfidData[1] = (0b00011110 & buffer[2]) << 3 | buffer[3] >> 4;
    rfidData[2] = buffer[3] << 5 | (0b10000000 & buffer[4]) >> 3 | (0b00111100 & buffer[4]) >> 2;
    rfidData[3] = buffer[4] << 7 | (0b11100000 & buffer[5]) >> 1 | 0b1111 & buffer[5];
    rfidData[4] = (0b01111000 & buffer[6]) << 1 | (0b11 & buffer[6]) << 2 | buffer[7] >> 6;
    return true;
}

byte ttAComp(unsigned long timeOut = 7000)                      // pulse 0 or 1 or -1 if timeout
{ 
    byte AcompState, AcompInitState;
    unsigned long tEnd = micros() + timeOut;
    AcompInitState = bitRead(ACSR, ACO);                        // читаем флаг компаратора
    do
    {
        AcompState = bitRead(ACSR, ACO);                        // читаем флаг компаратора
        if (AcompState != AcompInitState)
        {
            delayMicroseconds(1000 / (rfidBitRate * 4));        // 1/4 Period on 2 kBps = 125 mks
            AcompState = bitRead(ACSR, ACO);                    // читаем флаг компаратора
            delayMicroseconds(1000 / (rfidBitRate * 2));        // 1/2 Period on 2 kBps = 250 mks
            return AcompState;
        }
    } while (micros() < tEnd);
    return 2;                                                   // таймаут, компаратор не сменил состояние
}

bool readRFID(byte *buffer)
{
    unsigned long tEnd = millis() + 50;
    byte ti;
    byte j = 0, k = 0;
    for (int i = 0; i < 64; i++)                                // читаем 64 bit
    { 
        ti = ttAComp();
        if (ti == 2) break;                                     // timeout
        if ((ti == 0) && (i < 9))                               // если не находим 9 стартовых единиц - начинаем сначала
        { 
            if (millis() > tEnd)
            {
                ti = 2;
                break;
            } //timeout
            i = -1;
            j = 0;
            continue;
        }
        if ((i > 8) && (i < 59))                                // начиная с 9-го бита проверяем контроль четности каждой строки
        { 
            if (ti) k++;                                        // считаем кол-во единиц
            if ((i - 9) % 5 == 4)                               // конец строки с данными из 5-и бит,
            { 
                if (k & 1)                                      // если нечетно - начинаем сначала
                { 
                    i = -1;
                    j = 0;
                    k = 0;
                    continue;
                }
                k = 0;
            }
        }
        if (ti) bitSet(buffer[i >> 3], 7 - j);
            else bitClear(buffer[i >> 3], 7 - j);
        j++;
        if (j > 7) j = 0;
    }
    if (ti == 2) return false;                                  // timeout
    return vertEvenCheck(buffer);
}

void rfidACsetOn()                                              // включаем генератор 125кГц
{
    pinMode(RFID_PIN, OUTPUT);
    TCCR2A = bit(COM2A0) | bit(COM2B1) | bit(WGM21) | bit(WGM20); // Включаем режим Toggle on Compare Match на COM2A (pin 11) и счет таймера2 до OCR2A
    TCCR2B = bit(WGM22) | bit(CS20);                            // Задаем делитель для таймера2 = 1 (16 мГц)
    OCR2A = 31;                                                 // 63 тактов на период. Частота на COM2A (pin 11) 16000/64/2 = 125 кГц, Скважнось COM2A в этом режиме всегда 50%
    OCR2B = 15;                                                 // Скважность COM2B 32/64 = 50%  Частота на COM2A (pin 3) 16000/64 = 250 кГц //31 15???
    bitClear(ADCSRB, ACME);                                     // отключаем мультиплексор AC
    bitClear(ACSR, ACBG);                                       // отключаем от входа Ain0 1.1V
}

bool searchRFID(bool copyKey = true)
{
    bool result = false;
    rfidACsetOn();                                              // включаем генератор 125кГц и компаратор
    delay(6);                                                   // переходные процессы детектора
    if (!readRFID(addr))
    {
        if (!copyKey)
            TCCR2A &= 0b00111111;                               // Оключить ШИМ COM2A (pin 11)
        return result;
    }
    result = true;
    for (byte i = 0; i < 8; i++)
    {
        if (copyKey) keyID[i] = addr[i];
    }
    // debug data
    //   Serial.print(F(" ( id "));
    //   Serial.print(rfidData[0]); Serial.print(" key ");
    // unsigned long keyNum = (unsigned long)rfidData[1] << 24 | (unsigned long)rfidData[2] << 16 | (unsigned long)rfidData[3] << 8 | (unsigned long)rfidData[4];
    //   Serial.print(keyNum);
    //   Serial.println(F(") Type: EM-Marie "));
    if (!copyKey) TCCR2A &= 0b00111111;                         // Оключить ШИМ COM2A (pin 11)
    return result;
}

void rfidGap(unsigned int tm)                                   //  Restart PWM on COM2A (pin 11)
{
    TCCR2A &= 0b00111111; 
    delayMicroseconds(tm);
    bitSet(TCCR2A, COM2A0);
}

void txBitRfid(byte data)
{
    if (data & 1) 
        delayMicroseconds(54 * 8);
    else
        delayMicroseconds(24 * 8);
    rfidGap(19 * 8);
}

bool sendOpT5557(byte opCode, unsigned long password = 0, byte lockBit = 0, unsigned long data = 0, byte blokAddr = 1)
{
    txBitRfid(opCode >> 1);
    txBitRfid(opCode & 1);
    if (opCode == 0b00) return true;
    txBitRfid(lockBit & 1);
    if (data != 0)
    {
        for (byte i = 0; i < 32; i++)
        {
            txBitRfid((data >> (31 - i)) & 1);
        }
    }
    txBitRfid(blokAddr >> 2);
    txBitRfid(blokAddr >> 1);
    txBitRfid(blokAddr & 1); 
    delay(4);               
    return true;
}

bool T5557_blockRead(byte* buffer) {
  byte ti; 
  byte j = 0, k = 0;
  for (int i = 0; i < 33; i++)
  {                                                             // читаем стартовый 0 и 32 значащих bit
    ti = ttAComp(2000);
    if (ti == 2) break;                                         // timeout
    if ((ti == 1) && (i == 0))                                  // если не находим стартовый 0 - это ошибка
    { 
        ti = 2; break; 
    }     
    if (i > 0)                                                  // начиная с 1-го бита пишем в буфер
    {
      if (ti) bitSet(buffer[(i - 1) >> 3], 7 - j);
        else bitClear(buffer[(i - 1) >> 3], 7 - j);
      j++; 
      if (j > 7) j = 0;
    }
  }
  if (ti == 2) return false;                                    // timeout
  return true;
}

bool checkWriteState() {
  unsigned long data32, data33; 
  byte buffer[4] = {0, 0, 0, 0}; 
  rfidACsetOn();                                                // включаем генератор 125кГц и компаратор
  delay(13);                                                    // 13 мс длятся переходные процессы детектора
  rfidGap(30 * 8);                                              // start gap
  sendOpT5557(0b11, 0, 0, 0, 1);                                // переходим в режим чтения Vendor ID 
  if (!T5557_blockRead(buffer)) return false; 
  data32 = (unsigned long)buffer[0] << 24 | (unsigned long)buffer[1] << 16 | (unsigned long)buffer[2] << 8 | (unsigned long)buffer[3];
  delay(4);
  rfidGap(20 * 8);                                        
  data33 = 0b00000000000101001000000001000000 | (0 << 4);       // конфиг регистр 0b00000000000101001000000001000000
  sendOpT5557(0b10, 0, 0, data33, 0);                           // передаем конфиг регистр
  delay(4);
  rfidGap(30 * 8);                                              // start gap
  sendOpT5557(0b11, 0, 0, 0, 1);                                // переходим в режим чтения Vendor ID 
  if (!T5557_blockRead(buffer)) return false; 
  data33 = (unsigned long)buffer[0] << 24 | (unsigned long)buffer[1] << 16 | (unsigned long)buffer[2] << 8 | (unsigned long)buffer[3];
  sendOpT5557(0b00, 0, 0, 0, 0);                                // send Reset
  delay(6);
  if (data32 != data33) return false;    
//   Serial.print(F(" The rfid RW-key is T5557. Vendor ID is "));
//   Serial.println(data32, HEX);
  return true;
}

bool write2rfidT5557(byte *buffer)
{
    bool result;
    unsigned long data32;
    delay(6);
    for (byte k = 0; k < 2; k++)
    { // send key data
        data32 = (unsigned long)buffer[0 + (k << 2)] << 24 | (unsigned long)buffer[1 + (k << 2)] << 16 | (unsigned long)buffer[2 + (k << 2)] << 8 | (unsigned long)buffer[3 + (k << 2)];
        rfidGap(30 * 8);                                        // start gap
        sendOpT5557(0b10, 0, 0, data32, k + 1);                 // передаем 32 бита ключа в block k
        delay(6);
    }
    delay(6);
    rfidGap(30 * 8);                                            // start gap
    sendOpT5557(0b00);
    delay(4);
    result = readRFID(addr);
    TCCR2A &= 0b00111111;                                       // Оключить ШИМ COM2A (pin 11)
    for (byte i = 0; i < 8; i++)
    {
        if (addr[i] != keyID[i])
        {
            result = false;
            break;
        }
    }
    return result;
}

byte write2rfid()
{
    bool check = true;
    if (searchRFID(false))
    {
        for (byte i = 0; i < 8; i++)
            if (addr[i] != keyID[i])                            // сравниваем код для записи с тем, что уже записано в ключе.
            {
                check = false;
                break;
            }  
        if (check)                                                 // если коды совпадают, ничего писать не нужно
        {       
            return 3;                                           // Error - same key
        }                                   
    }
    
    if (checkWriteState()) 
    {
        if (write2rfidT5557(keyID)) return 1;                   // Write ok
            else return 2;                                      // Write error
                                              
    }
    return 0;                                                   // Idle
}

void sendEM_Marine(byte *buffer)                                // send key (Test version)
{
    TCCR2A &= 0b00111111;                                   
    bitClear(PORTB, 3);
    delay(20);
    for (byte k = 0; k < 10; k++)
    {
        for (byte i = 0; i < 8; i++)
        {
            for (byte j = 0; j < 8; j++)
            {
                if (1 & (buffer[i] >> (7 - j)))
                {
                    bitSet(DDRB, 3);
                    delayMicroseconds(250);
                    bitClear(DDRB, 3);
                    delayMicroseconds(250);
                }
                else
                {
                    bitClear(DDRB, 3);
                    delayMicroseconds(250);
                    bitSet(DDRB, 3);
                    delayMicroseconds(250);
                }
            }
        }
    }
}
