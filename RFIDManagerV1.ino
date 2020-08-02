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

#define LCD_CLK A0
#define LCD_DIN A1
#define LCD_DC A2
#define LCD_CE A3
#define LCD_RST A4

#define BUTTON_PIN 13                           // Sensor button

#define delayAction 800                         // Delay between actions
#define delayWrite 1000                         // Delay between writtings

#define batPit A7                               // Pin for voltage monitoring

//-------------------- Objects init --------------------
GButton button(BUTTON_PIN, LOW_PULL, NORM_OPEN);
Adafruit_PCD8544 display = Adafruit_PCD8544(LCD_CLK, LCD_DIN, LCD_DC, LCD_CE, LCD_RST);

uint32_t actionTimeStamp;                       // Timestamp on action
uint32_t writeTimeStamp;                        // Timestamp on write
uint8_t writeStatus;                            // Write status ()
boolean hasReadKey = false;                     // Reak key status(true -> did read)

volatile uint32_t timerMills;                   // Counter mills for timer0
int32_t timerTimestamp;                         // Timestamp on button reading
int32_t displayTipestamp;                       // Timestamp on display refreshing

enum Mode {                                     // Modes 
    read, write, emulator 
} mode;

ISR (TIMER0_COMPA_vect) {                       // Timer0 interrupt 
    timerMills++;                               // Increment counter
    if (timerMills - timerTimestamp >= 10) {    // If less more 20ms -> read button
        timerTimestamp = timerMills;
        button.tick();
    }

    if (timerMills - displayTipestamp >= 100) { // If less more 100ms -> refresh info on display
        displayTipestamp = timerMills;
        refreshDisplay();
    }
}

void setup() {
                                                // TIMER0 Setup
    bitSet(TCCR0A, WGM01);                      // Reset on compare
    OCR0A = 249;                                // Begin count on overflow
    bitSet(TIMSK0, OCIE0A);                     // Enable interrupt if compare with register A
    bitSet(TCCR0B, CS01);                       // Set clock divider on 64
    bitSet(TCCR0B, CS00);
    sei();                                      // Enable global interrupts

    mode = read;                                // Select start mode
    
    Serial.begin(115200);                       // For debug
                                                // EEPROM Setup
    EEPROM_key_count = EEPROM[0];               // Get count of keys 
    maxKeyCount = EEPROM.length() / 8 - 1;      // Get maximun count of keys
    if (maxKeyCount > 20) {                     // It doesn't be over 20
        maxKeyCount = 20;
    } 
    if (EEPROM_key_count > maxKeyCount) {       //
        EEPROM_key_count = 0;
    }

    analogReference(INTERNAL);                  // Initialise internal reference for voltage monitor

    setupDisplay();                             // Setup display
    refreshDisplay();                           // Refresh info on display
}

void loop() {
    readButton();                               // Read buttons  
    if (millis() - actionTimeStamp < delayAction) { return; }
    actionTimeStamp = millis();                 // Timeout between operations
    action();                                   // Do operation
}

void readButton() {
    if (button.isHold()) { saveKey(); }         // Hold key -- Save key to EEPROM

    if (button.hasClicks()) 
    {
        switch (button.getClicks()) {
        case 1:                                 // Single tap -> Change mode
            switch (mode) {
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
            test1();
            test2();
            break;
        case 2:                                 // Double tap -> Next key from EEPROM
            nextKey();
            break;
        case 3:                                 // Triple tap -> Previous key from EEPROM
            prevKey();
            break;
        case 7:                                 // Seven tap -- Clear EEPROM
            EEPROM.update(0, 0); 
            EEPROM.update(1, 0);
            EEPROM_key_count = 0; 
            EEPROM_key_index = 0;
            hasReadKey = false;
            break;        
        default:
            break;
        }
    }
}

void action() {                                 // Do action
    switch (mode) {
    case read:
        if (searchRFID(true)) {
            hasReadKey = true;
        }
        break;
    case write: 
        writeAction();
        break;
    case emulator: 
        sendEM_Marine(keyID);
        break;
    }
}

void setupDisplay() {                           // Setup display
    display.begin();
    display.setContrast(50);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK);
}

void nextKey() {                                // Next key handling tapped
    if (EEPROM_key_count > 0) {
        EEPROM_key_index++;
        if (EEPROM_key_index > EEPROM_key_count) { 
            EEPROM_key_index = 1; 
        }
        EEPROM_get_key(EEPROM_key_index, keyID);
        EEPROM.update(1, EEPROM_key_index);
    }
    hasReadKey = false;
}

void prevKey() {                                // Previous key handling tapped
    if (EEPROM_key_count > 0) {
        EEPROM_key_index--;
        if (EEPROM_key_index < 1) { 
            EEPROM_key_index = EEPROM_key_count; 
        }
        EEPROM_get_key(EEPROM_key_index, keyID);
        EEPROM.update(1, EEPROM_key_index);
    }
    hasReadKey = false;
}

void saveKey() {                                // Save key handling
    EEPROM_AddKey(keyID);
    hasReadKey = false;
}

void writeAction() {
    if (millis() - writeTimeStamp < delayWrite) { return; }
    writeTimeStamp = millis();
    writeStatus = write2rfid();
}

void refreshDisplay() {
    display.clearDisplay();
    display.setCursor(0, 0);
    if (hasReadKey) {
        for (byte i = 0; i < strlen_P(key_txt); i++) {
           display.print((char)pgm_read_byte(&key_txt[i]));
        }
        for (byte i = 0; i < 8; i++) {
            display.print(keyID[i], HEX);
            if (i != 7) { display.print(F(":")); }
        }
    } else if (EEPROM_key_count != 0) {
        EEPROM_key_index = EEPROM[1];
        EEPROM_get_key(EEPROM_key_index, keyID);
        display.print(F("["));
        display.print(EEPROM_key_index);
        display.print(F(":"));
        display.print(EEPROM_key_count);
        display.print(F("]"));
        for (byte i = 0; i < 8; i++) {
            display.print(keyID[i], HEX);
            if (i != 7) { display.print(F(":")); }
        }
    } else {
        for (byte i = 0; i < strlen_P(noKeys_txt); i++) {
           display.print((char)pgm_read_byte(&noKeys_txt[i]));
        }
    }

    display.println();
    display.println();

    if (mode == write) {
        switch (writeStatus) {
        case 1:
            for (byte i = 0; i < strlen_P(writeOK_txt); i++) {
                display.print((char)pgm_read_byte(&writeOK_txt[i]));
            }
            break;
        case 2: 
            for (byte i = 0; i < strlen_P(writeFailed_txt); i++) {
                display.print((char)pgm_read_byte(&writeFailed_txt[i]));
            }
            break;
        case 3:
            for (byte i = 0; i < strlen_P(sameKey_txt); i++) {
                display.print((char)pgm_read_byte(&sameKey_txt[i]));
            }
            break;
        default:
            for (byte i = 0; i < strlen_P(readyToWrite_txt); i++) {
                display.print((char)pgm_read_byte(&readyToWrite_txt[i]));
            }
            break;
        }
        display.println();
    }
    display.setCursor(0,41);
    float Vin = (analogRead(batPit) * 1.1) / 1023 / 0.094;
    display.print(F("V="));
    display.print(Vin);

    display.setCursor(78,41);
    switch (mode) {
    case read:
        display.print(F("R"));
        break;
    case write:
        display.print(F("W"));
        break;
    case emulator:
        display.print(F("E"));
        break;
    }

    display.display();
}