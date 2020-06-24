#pragma once
#include <Arduino.h>
#include <EEPROM.h>

byte maxKeyCount;                           // максимальное кол-во ключей, которое влазит в EEPROM, но не > 20
byte EEPROM_key_count;                      // количество ключей 0..maxKeyCount, хранящихся в EEPROM
byte EEPROM_key_index = 0;                  // 1..EEPROM_key_count номер последнего записанного в EEPROM ключа 

bool isKeyEmpty(byte buffer[]) 
{
    byte result = 0;
    for (byte i = 0; i < 8; i++) 
    {
        result += buffer[i];
    }
    return (result == 0);
}

byte indexKeyInROM(byte buffer[])           // возвращает индекс или ноль если нет в ROM
{ 
    byte buffer1[8]; 
    bool eq = true;
    for (byte j = 1; j <= EEPROM_key_count; j++)
    {  // ищем ключ в eeprom. 
        EEPROM.get(j * sizeof(buffer1), buffer1);
        for (byte i = 0; i < 8; i++) 
            if (buffer1[i] != buffer[i]) 
            { 
                eq = false;
                break; 
            }
        if (eq) return j;
        eq = true;
    }
    return 0;
}

bool EEPROM_AddKey(byte buffer[]) 
{

    if (isKeyEmpty(buffer)) return false;

    byte buffer1[8]; 
    byte index = indexKeyInROM(buffer);
              
    if (index != 0) // ищем ключ в eeprom. Если находим, то не делаем запись, а индекс переводим в него
    { 
        EEPROM_key_index = index;
        EEPROM.update(1, EEPROM_key_index);
        return false; 
    }

    if (EEPROM_key_count <= maxKeyCount) EEPROM_key_count++;
    if (EEPROM_key_count < maxKeyCount) EEPROM_key_index = EEPROM_key_count;
        else EEPROM_key_index++;
    if (EEPROM_key_index > EEPROM_key_count) EEPROM_key_index = 1;
    // Serial.println(F("Adding to EEPROM"));
    for (byte i = 0; i < 8; i++) 
    {
        buffer1[i] = buffer[i];
        // Serial.print(buffer[i], HEX); Serial.print(F(":"));  
    }
    // Serial.println();
    EEPROM.put(EEPROM_key_index * sizeof(buffer1), buffer1);
    EEPROM.update(0, EEPROM_key_count);
    EEPROM.update(1, EEPROM_key_index);
    return true;
}

void EEPROM_get_key(byte EEPROM_key_index1, byte buffer[8])
{
  byte buffer1[8];
  int address = EEPROM_key_index1 * sizeof(buffer1);
  if (address > EEPROM.length()) return;
  EEPROM.get(address, buffer1);
  for (byte i = 0; i < 8; i++) buffer[i] = buffer1[i];
}