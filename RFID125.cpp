#include "RFID125.h"
#include <Arduino.h>

byte rfidData[5]; // значащие данные frid em-marine
byte keyID[8];    // ID ключа для записи
byte addr[8];     // временный буфер

// Check on even cols with data
bool vertEvenCheck(byte *buf)
{
    byte k;
    k = 1 & buf[1] >> 6 + 1 & buf[1] >> 1 + 1 & buf[2] >> 4 + 1 & buf[3] >> 7 + 1 & buf[3] >> 2 + 1 & buf[4] >> 5 + 1 & buf[4] + 1 & buf[5] >> 3 + 1 & buf[6] >> 6 + 1 & buf[6] >> 1 + 1 & buf[7] >> 4;
    if (k & 1)
        return false;
    k = 1 & buf[1] >> 5 + 1 & buf[1] + 1 & buf[2] >> 3 + 1 & buf[3] >> 6 + 1 & buf[3] >> 1 + 1 & buf[4] >> 4 + 1 & buf[5] >> 7 + 1 & buf[5] >> 2 + 1 & buf[6] >> 5 + 1 & buf[6] + 1 & buf[7] >> 3;
    if (k & 1)
        return false;
    k = 1 & buf[1] >> 4 + 1 & buf[2] >> 7 + 1 & buf[2] >> 2 + 1 & buf[3] >> 5 + 1 & buf[3] + 1 & buf[4] >> 3 + 1 & buf[5] >> 6 + 1 & buf[5] >> 1 + 1 & buf[6] >> 4 + 1 & buf[7] >> 7 + 1 & buf[7] >> 2;
    if (k & 1)
        return false;
    k = 1 & buf[1] >> 3 + 1 & buf[2] >> 6 + 1 & buf[2] >> 1 + 1 & buf[3] >> 4 + 1 & buf[4] >> 7 + 1 & buf[4] >> 2 + 1 & buf[5] >> 5 + 1 & buf[5] + 1 & buf[6] >> 3 + 1 & buf[7] >> 6 + 1 & buf[7] >> 1;
    if (k & 1)
        return false;
    if (1 & buf[7])
        return false;
    //номер ключа, который написан на корпусе
    rfidData[0] = (0b01111000 & buf[1]) << 1 | (0b11 & buf[1]) << 2 | buf[2] >> 6;
    rfidData[1] = (0b00011110 & buf[2]) << 3 | buf[3] >> 4;
    rfidData[2] = buf[3] << 5 | (0b10000000 & buf[4]) >> 3 | (0b00111100 & buf[4]) >> 2;
    rfidData[3] = buf[4] << 7 | (0b11100000 & buf[5]) >> 1 | 0b1111 & buf[5];
    rfidData[4] = (0b01111000 & buf[6]) << 1 | (0b11 & buf[6]) << 2 | buf[7] >> 6;
    return true;
}

byte ttAComp(unsigned long timeOut = 7000)
{ // pulse 0 or 1 or -1 if timeout
    byte AcompState, AcompInitState;
    unsigned long tEnd = micros() + timeOut;
    AcompInitState = bitRead(ACSR, ACO); // читаем флаг компаратора
    do
    {
        AcompState = bitRead(ACSR, ACO);                 // читаем флаг компаратора
        if (AcompState != AcompInitState)
        {
            delayMicroseconds(1000 / (rfidBitRate * 4)); // 1/4 Period on 2 kBps = 125 mks
            AcompState = bitRead(ACSR, ACO);             // читаем флаг компаратора
            delayMicroseconds(1000 / (rfidBitRate * 2)); // 1/2 Period on 2 kBps = 250 mks
            return AcompState;
        }
    } while (micros() < tEnd);
    return 2; //таймаут, компаратор не сменил состояние
}

bool readEM_Marie(byte *buf)
{
    unsigned long tEnd = millis() + 50;
    byte ti;
    byte j = 0, k = 0;
    for (int i = 0; i < 64; i++)
    { // читаем 64 bit
        ti = ttAComp();
        if (ti == 2)
            break; //timeout
        if ((ti == 0) && (i < 9))
        { // если не находим 9 стартовых единиц - начинаем сначала
            if (millis() > tEnd)
            {
                ti = 2;
                break;
            } //timeout
            i = -1;
            j = 0;
            continue;
        }
        if ((i > 8) && (i < 59))
        { //начиная с 9-го бита проверяем контроль четности каждой строки
            if (ti)
                k++; // считаем кол-во единиц
            if ((i - 9) % 5 == 4)
            { // конец строки с данными из 5-и бит,
                if (k & 1)
                { //если нечетно - начинаем сначала
                    i = -1;
                    j = 0;
                    k = 0;
                    continue;
                }
                k = 0;
            }
        }
        if (ti)
            bitSet(buf[i >> 3], 7 - j);
        else
            bitClear(buf[i >> 3], 7 - j);
        j++;
        if (j > 7)
            j = 0;
    }
    if (ti == 2)
        return false; //timeout
    return vertEvenCheck(buf);
}

void rfidACsetOn()
{
    //включаем генератор 125кГц
    pinMode(FrequencyGen, OUTPUT);
    TCCR2A = bit(COM2A0) | bit(COM2B1) | bit(WGM21) | bit(WGM20); //Вкючаем режим Toggle on Compare Match на COM2A (pin 11) и счет таймера2 до OCR2A
    TCCR2B = bit(WGM22) | bit(CS20);                              // Задаем делитель для таймера2 = 1 (16 мГц)
    OCR2A = 63;                                                   // 63 тактов на период. Частота на COM2A (pin 11) 16000/64/2 = 125 кГц, Скважнось COM2A в этом режиме всегда 50%
    OCR2B = 31;                                                   // Скважность COM2B 32/64 = 50%  Частота на COM2A (pin 3) 16000/64 = 250 кГц //31 15???
    // включаем компаратор
    bitClear(ADCSRB, ACME); // отключаем мультиплексор AC
    bitClear(ACSR, ACBG);   // отключаем от входа Ain0 1.1V
}

bool searchEM_Marine(bool copyKey = true)
{
    bool result = false;
    rfidACsetOn(); // включаем генератор 125кГц и компаратор
    delay(6);      //13 мс длятся переходные прцессы детектора
    if (!readEM_Marie(addr))
    {
        if (!copyKey)
            TCCR2A &= 0b00111111; //Оключить ШИМ COM2A (pin 11)
        return result;
    }
    result = true;
    for (byte i = 0; i < 8; i++)
    {
        if (copyKey)
            keyID[i] = addr[i];
        // Serial.print(addr[i], HEX); Serial.print(":");
    }
    //   Serial.print(F(" ( id "));
    //   Serial.print(rfidData[0]); Serial.print(" key ");
    unsigned long keyNum = (unsigned long)rfidData[1] << 24 | (unsigned long)rfidData[2] << 16 | (unsigned long)rfidData[3] << 8 | (unsigned long)rfidData[4];
    //   Serial.print(keyNum);
    //   Serial.println(F(") Type: EM-Marie "));
    if (!copyKey)
        TCCR2A &= 0b00111111; // Оключить ШИМ COM2A (pin 11)
    return result;
}

void rfidGap(unsigned int tm)
{
    TCCR2A &= 0b00111111; // Оключить ШИМ COM2A
    delayMicroseconds(tm);
    bitSet(TCCR2A, COM2A0); // Включить ШИМ COM2A (pin 11)
}

void TxBitRfid(byte data)
{
    if (data & 1)
        delayMicroseconds(54 * 8);
    else
        delayMicroseconds(24 * 8);
    rfidGap(19 * 8); //write gap
}

void TxByteRfid(byte data)
{
    for (byte n_bit = 0; n_bit < 8; n_bit++)
    {
        TxBitRfid(data & 1);
        data = data >> 1; // переходим к следующему bit
    }
}

bool sendOpT5557(byte opCode, unsigned long password = 0, byte lockBit = 0, unsigned long data = 0, byte blokAddr = 1)
{
    TxBitRfid(opCode >> 1);
    TxBitRfid(opCode & 1); // передаем код операции 10
    if (opCode == 0b00)
        return true;
    // password
    TxBitRfid(lockBit & 1); // lockbit 0
    if (data != 0)
    {
        for (byte i = 0; i < 32; i++)
        {
            TxBitRfid((data >> (31 - i)) & 1);
        }
    }
    TxBitRfid(blokAddr >> 2);
    TxBitRfid(blokAddr >> 1);
    TxBitRfid(blokAddr & 1); // адрес блока для записи
    delay(4);                // ждем пока пишутся данные
    return true;
}

bool write2rfidT5557(byte *buf)
{
    bool result;
    unsigned long data32;
    delay(6);
    for (byte k = 0; k < 2; k++)
    { // send key data
        data32 = (unsigned long)buf[0 + (k << 2)] << 24 | (unsigned long)buf[1 + (k << 2)] << 16 | (unsigned long)buf[2 + (k << 2)] << 8 | (unsigned long)buf[3 + (k << 2)];
        rfidGap(30 * 8);                        //start gap
        sendOpT5557(0b10, 0, 0, data32, k + 1); //передаем 32 бита ключа в blok k
        // Serial.print('*');
        delay(6);
    }
    delay(6);
    rfidGap(30 * 8); //start gap
    sendOpT5557(0b00);
    delay(4);
    result = readEM_Marie(addr);
    TCCR2A &= 0b00111111; //Оключить ШИМ COM2A (pin 11)
    for (byte i = 0; i < 8; i++)
        if (addr[i] != keyID[i])
        {
            result = false;
            break;
        }
    if (!result)
    {
        // Serial.println(F(" The key copy faild"));
        // OLED_printError(F("The key copy faild"));
        // Sd_ErrorBeep();
    }
    else
    {
        // Serial.println(F(" The key has copied successesfully"));
        // OLED_printError(F("The key has copied"), false);
        // Sd_ReadOK();
        delay(2000);
    }
    //   digitalWrite(R_Led, HIGH);
    return result;
}

bool write2rfid()
{
    bool check = true;
    if (searchEM_Marine(false))
    {
        for (byte i = 0; i < 8; i++)
            if (addr[i] != keyID[i])
            {
                check = false;
                break;
            } // сравниваем код для записи с тем, что уже записано в ключе.
        if (check)
        {   // если коды совпадают, ничего писать не нужно
            //   digitalWrite(R_Led, LOW);
            //   Serial.println(F(" it is the same key. Writing in not needed."));
            //   OLED_printError(F("It is the same key"));
            //   Sd_ErrorBeep();
            //   digitalWrite(R_Led, HIGH);
            delay(1000);
            return false;
        }
    }
    write2rfidT5557(keyID); //пишем T5557

    return false;
}

void SendEM_Marine(byte *buf)
{
    TCCR2A &= 0b00111111; // отключаем шим
    digitalWrite(FrequencyGen, LOW);
    //FF:A9:8A:A4:87:78:98:6A
    delay(20);
    for (byte k = 0; k < 10; k++)
    {
        for (byte i = 0; i < 8; i++)
        {
            for (byte j = 0; j < 8; j++)
            {
                if (1 & (buf[i] >> (7 - j)))
                {
                    pinMode(FrequencyGen, INPUT);
                    delayMicroseconds(250);
                    pinMode(FrequencyGen, OUTPUT);
                    delayMicroseconds(250);
                }
                else
                {
                    pinMode(FrequencyGen, OUTPUT);
                    delayMicroseconds(250);
                    pinMode(FrequencyGen, INPUT);
                    delayMicroseconds(250);
                }
            }
        }
    }
}
