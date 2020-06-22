#pragma once
#include <Arduino.h>

void myDelayMicroseconds(uint32_t us) 
{
    uint32_t tmr = micros();
    while (micros() - tmr < us);
}

void myDelay(uint32_t ms) {
    uint32_t tmr = millis();
    while (millis() - tmr < ms);
}