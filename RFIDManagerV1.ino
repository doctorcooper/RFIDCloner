#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <GyverButton.h>
#include <EEPROM.h>
#include "RFID125.h"

//-------------------- Pins define --------------------
// Nokia display - Adafruit_PCD8544(CLK,DIN,D/C,CE,RST);

#define LCD_CLK A4
#define LCD_DIN A3
#define LCD_DC A2
#define LCD_CE A1
#define LCD_RST A0

// Sensor button
#define BUTTON_PIN 10

#define delayAction 500

//-------------------- Objects init --------------------
GButton button(BUTTON_PIN);
Adafruit_PCD8544 display = Adafruit_PCD8544(LCD_CLK, LCD_DIN, LCD_DC, LCD_CE, LCD_RST);

unsigned long sequenceTimer = millis();

enum Mode
{
    read,
    write,
    emulator
} mode;

void setup()
{
    setupDisplay();
    button.setTickMode(AUTO);
    mode = read;
    setReadMode(); // TODO: - save mode to EEPROM and read it on start
    Serial.begin(115200);
}

void loop()
{
    readButton();
    if (millis() - sequenceTimer < delayAction)
        return;
    sequenceTimer = millis();
    action();
}

void readButton()
{
    // Single tap -> Change mode
    if (button.isSingle())
    {
        display.clearDisplay();
        switch (mode)
        {
        case read:
            setWriteMode();
            break;
        case write:
            setEmulatorMode();
            break;
        case emulator:
            setReadMode();
            break;
        }
    }
    // Double tap -> Next key from EEPROM
    if (button.isDouble())
    {
        nextKey();
    }
    // Triple tap -> Previous key from EEPROM
    if (button.isTriple())
    {
        prevKey();
    }
    // Hold key -> Save key from buffer to EEPROM
    if (button.isHold())
    {
        saveKey();
    }
}

void action()
{
    switch (mode)
        {
        case read:
            if (searchEM_Marine(true)) showKeyID();
            break;
        case write:
            write2rfid();
            break;
        case emulator:
            SendEM_Marine(keyID);
            break;
        }
}

void setupDisplay()
{
    display.begin();
    display.setContrast(27);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK);
}

void setReadMode()
{
    display.setCursor(0, 0);
    display.println("Read mode");
    display.display();
    mode = read;
}

void setWriteMode()
{
    display.setCursor(0, 0);
    display.println("Write mode");
    display.display();
    mode = write;
}

void setEmulatorMode()
{
    display.setCursor(0, 0);
    display.println("Emulator mode");
    display.display();
    mode = emulator;
}

void nextKey()
{
}

void prevKey()
{
}

void saveKey()
{
}

void showKeyID() {
    String key = "";
    for (byte i = 0; i < 8; i++) 
    {
        key += String(keyID[i], HEX);
        if (i != 7) key += ":";
    }
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(key);
    display.display();
}

