#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <GyverButton.h>
#include "RFID125.h"
#include "Strings.h"
#include "EEPROMHelper.h"

//-------------------- Pins define --------------------
// Nokia display - Adafruit_PCD8544(CLK,DIN,D/C,CE,RST);

#define LCD_CLK A4
#define LCD_DIN A3
#define LCD_DC A2
#define LCD_CE A1
#define LCD_RST A0

// Sensor button
#define BUTTON_PIN 10

#define delayAction 800
#define delayWrite 1000

//-------------------- Objects init --------------------
GButton button(BUTTON_PIN);
Adafruit_PCD8544 display = Adafruit_PCD8544(LCD_CLK, LCD_DIN, LCD_DC, LCD_CE, LCD_RST);

unsigned long actionTimeStamp = millis();
unsigned long writeTimeStamp = millis();
byte writeStatus;
boolean isReadedKey = false;

enum Mode { read, write, emulator } mode;

void setup()
{
    button.setTickMode(AUTO);
    mode = read; // TODO: - save mode to EEPROM and read it on start
    setupDisplay(); 
    Serial.begin(115200); // For debug

    EEPROM_key_count = EEPROM[0];
    maxKeyCount = EEPROM.length() / 8 - 1; 
    if (maxKeyCount > 20) maxKeyCount = 20;
    if (EEPROM_key_count > maxKeyCount) EEPROM_key_count = 0;
    refreshDisplay();
}

void loop()
{
    readButton();
    if (millis() - actionTimeStamp < delayAction) return;
    actionTimeStamp = millis();
    action();
}

void readButton()
{
    // Single tap -> Change mode
    if (button.isSingle())
    {
        switch (mode)
        {
        case read: 
            mode = write;
            break;
        case write:
            mode = emulator;
            break;
        case emulator:
            mode = read;
            break;
        }
        refreshDisplay();
    }
    // Double tap -> Next key from EEPROM
    if (button.isDouble()) nextKey();
    // Triple tap -> Previous key from EEPROM
    if (button.isTriple()) prevKey();
    // Hold key -> Save key from buffer to EEPROM
    if (button.isHold()) saveKey();

    // if (button.hasClicks()) {
    //     if (button.getClicks() == 7) {
    //         Serial.println(F("EEPROM cleared"));
    //         EEPROM.update(0, 0); 
    //         EEPROM.update(1, 0);
    //         EEPROM_key_count = 0; 
    //         EEPROM_key_index = 0;
    //     }
    //     isReadedKey = false;
    // }

}

void action()
{
    switch (mode)
    {
    case read:
        if (searchRFID(true)) 
        {
            isReadedKey = true;
            refreshDisplay();
        }
        break;
    case write: 
        writeAction();
        break;
    case emulator: sendEM_Marine(keyID);
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

void nextKey()
{    
    if (EEPROM_key_count > 0) {
        EEPROM_key_index++;
        if (EEPROM_key_index > EEPROM_key_count) EEPROM_key_index = 1;
        EEPROM_get_key(EEPROM_key_index, keyID);
        EEPROM.update(1, EEPROM_key_index);
    }
    isReadedKey = false;
    refreshDisplay();
}

void prevKey()
{
    
    if (EEPROM_key_count > 0) {
        EEPROM_key_index--;
        if (EEPROM_key_index < 1) EEPROM_key_index = EEPROM_key_count;
        EEPROM_get_key(EEPROM_key_index, keyID);
        EEPROM.update(1, EEPROM_key_index);
    }
    isReadedKey = false;
    refreshDisplay();
}

void saveKey()
{
    EEPROM_AddKey(keyID);
    delay(1000);
    isReadedKey = false;
    refreshDisplay();
}

void writeAction() 
{
    if (millis() - writeTimeStamp < delayWrite) return;
    writeTimeStamp = millis();
    writeStatus = write2rfid();
    refreshDisplay();
}

void refreshDisplay()
{
    display.clearDisplay();
    display.setCursor(0, 0);
    switch (mode)
    {
    case read:
        for (byte i = 0; i < strlen_P(readMode_txt); i++)
        {
            display.print((char)pgm_read_byte(&readMode_txt[i]));
        }
        break;
    case write:
        for (byte i = 0; i < strlen_P(writeMode_txt); i++)
        {
            display.print((char)pgm_read_byte(&writeMode_txt[i]));
        }
        break;
    case emulator:
        for (byte i = 0; i < strlen_P(emulatorMode_txt); i++)
        {
            display.print((char)pgm_read_byte(&emulatorMode_txt[i]));
        }
        break;
    }

    display.println();

    if (isReadedKey) 
    {
        for (byte i = 0; i < strlen_P(key_txt); i++)
        {
           display.print((char)pgm_read_byte(&key_txt[i]));
        }
        for (byte i = 0; i < 8; i++)
        {
            display.print(keyID[i], HEX);
            if (i != 7) display.print(F(":"));
        }
    } else if (EEPROM_key_count != 0) {
        EEPROM_key_index = EEPROM[1];
        EEPROM_get_key(EEPROM_key_index, keyID);
        display.print(F("["));
        display.print(EEPROM_key_index);
        display.print(F("of"));
        display.print(EEPROM_key_count);
        display.print(F("]"));
        for (byte i = 0; i < 8; i++)
        {
            display.print(keyID[i], HEX);
            if (i != 7) display.print(F(":"));
        }
    } else {
        for (byte i = 0; i < strlen_P(noKeys_txt); i++)
        {
           display.print((char)pgm_read_byte(&noKeys_txt[i]));
        }
    }

    display.println();

    if (mode == write) 
    {
        switch (writeStatus)
        {
        case 1:
            for (byte i = 0; i < strlen_P(writeOK_txt); i++)
            {
                display.print((char)pgm_read_byte(&writeOK_txt[i]));
            }
            break;
        case 2: 
            for (byte i = 0; i < strlen_P(writeFailed_txt); i++)
            {
                display.print((char)pgm_read_byte(&writeFailed_txt[i]));
            }
            break;
        case 3:
            for (byte i = 0; i < strlen_P(sameKey_txt); i++)
            {
                display.print((char)pgm_read_byte(&sameKey_txt[i]));
            }
            break;
        default:
            for (byte i = 0; i < strlen_P(readyToWrite_txt); i++)
            {
                display.print((char)pgm_read_byte(&readyToWrite_txt[i]));
            }
            break;
        }
        display.println();
    }
    display.setCursor(0,40);
    display.print(F("V=5.0v"));
    display.display();
}