#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <GyverButton.h>
#include "Strings.h"
#include "RFID-TAG-125kHz.h"
#include <Wire.h>
#include "KeysStorage.h"
#include <GyverTimer.h>

//-------------------- Pins define --------------------
// Nokia display - Adafruit_PCD8544(CLK,DIN,D/C,CE,RST);

#define LCD_CLK A0
#define LCD_DIN A1
#define LCD_DC A2
#define LCD_CE A3
#define LCD_RST A4
#define BUTTON_PIN 13                           // Sensor button

// #define DEBUG 0                              // Debug define
#define batPit A7                               // Pin for voltage monitoring

//-------------------- Objects init --------------------
GButton button(BUTTON_PIN, LOW_PULL, NORM_OPEN);
Adafruit_PCD8544 display = Adafruit_PCD8544(LCD_CLK, LCD_DIN, LCD_DC, LCD_CE, LCD_RST);
RFID_TAG reader = RFID_TAG(2);
KeysStorage storage = KeysStorage();

GTimer buttonTimer = GTimer(MS, 10);
GTimer displayTimer = GTimer(MS, 500);
GTimer rfidOperationTimer = GTimer(MS, 1000);

//-------------------- Variables -----------------------
uint8_t writeStatus;                            // Write status ()
bool hasReadKey = false;                        // Read key status(true -> did read)

enum Mode { read, write, emulator } mode;       // Modes

uint8_t keyUID[5];

void refreshDisplay() {
    display.clearDisplay();
    display.setCursor(0, 0);
    if (hasReadKey) {
        for (byte i = 0; i < 5; i++) {
            display.print(keyUID[i], HEX);
            if (i != 4) { display.print(F(":")); }
        }
        display.println();
        printString(read_txt);
    } else if (storage.getKeyCount() != 0) {
        storage.getKeyByIndex(keyUID);
        for (byte i = 0; i < 5; i++) {
            display.print(keyUID[i], HEX);
            if (i != 4) { display.print(F(":")); }
        }
        display.println();
        display.print(F("["));
        display.print(storage.getKeyIndex());
        display.print(F("of"));
        display.print(storage.getKeyCount());
        display.print(F("]"));
    } else {
        printString(noKeys_txt);
    }

    display.println();
    display.println();

    if (mode == write) {
        switch (writeStatus) {
        case 1:
            printString(writeOK_txt);
            break;
        case 2: 
            printString(writeFailed_txt);
            break;
        case 3:
            printString(sameKey_txt);
            break;
        default:
            printString(readyToWrite_txt);
            break;
        }
        display.println();
    }

    /*
    display.setCursor(0,41);
    float Vin = (analogRead(batPit) * 1.1) / 1023 / 0.094;
    display.print(F("V="));
    display.print(Vin);
    */

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

void nextKey() {                                // Next key handling tapped
    if (storage.getKeyCount() > 0) {
        storage.incrementIndex();
        storage.getKeyByIndex(keyUID);
    }
    hasReadKey = false;
}

void prevKey() {                                // Previous key handling tapped
    if (storage.getKeyCount() > 0) {
        storage.decrementIndex();
        storage.getKeyByIndex(keyUID);
    }
    hasReadKey = false;
}

void saveKey() {                                // Save key handling
    storage.addKey(keyUID);
    hasReadKey = false;
}

void readButton() {
    if (button.isHold()) { saveKey(); }         // Hold key -- Save key to EEPROM

    if (button.hasClicks()) 
    {
        switch (button.getClicks()) {
        case 1:                                 // Single tap -> Change mode
            switch (mode) {
            case read: 
                reader.emulatorEnabled = false;
                mode = write;
                
                break;
            case write:
                reader.emulatorEnabled = true;
                reader.prepareEmulator();
                mode = emulator;
                break;
            case emulator:
                reader.emulatorEnabled = false;
                mode = read;
                
                break;
            }
            break;
        case 2:                                 // Double tap -> Next key from EEPROM
            nextKey();
            break;
        case 3:                                 // Triple tap -> Previous key from EEPROM
            prevKey();
            break;
        case 7:                                 // Seven tap -- Clear EEPROM
            storage.clear();
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
        if (reader.searchTag(true) && reader.readOperationIsBusy) {
            reader.getUID(keyUID);
            hasReadKey = true;
        }
        break;
    case write: 
        if (reader.writeOperationIsBusy) { writeStatus = reader.writeTag(keyUID); } 
        break;
    case emulator: 
        reader.emulateKey(keyUID);
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

void setupTimer1() {
    noInterrupts();
    // Clear registers
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    // 3906.25 Hz (8000000/((2047+1)*1))
    OCR1A = 2047;
    // CTC
    TCCR1B |= (1 << WGM12);
    // Prescaler 1
    TCCR1B |= (1 << CS10);
    // Output Compare Match A Interrupt Enable
    TIMSK1 |= (1 << OCIE1A);
    interrupts();
}

volatile uint8_t bit_counter = 0;
volatile uint8_t byte_counter = 0;
volatile uint8_t half = 0;

ISR(TIMER1_COMPA_vect) {
    if (!reader.emulatorEnabled) { return; }
    TCNT1=0;
	if (((reader.getRawKey()[byte_counter] << bit_counter) & 0x80) == 0x00) {
	    if (half==0) pinMode(11, OUTPUT);
	    if (half==1) pinMode(11, INPUT);
	} else {
	    if (half==0) pinMode(11, INPUT);
	    if (half==1) pinMode(11, OUTPUT);
	}
    
	half++;
	if (half == 2) {
	    half = 0;
	    bit_counter++;
	    if (bit_counter == 8) {
	        bit_counter = 0;
	        byte_counter = (byte_counter + 1) % 8;
		}
	}
}

void setup() {
    mode = read;                                // Select start mode
    
    #ifdef DEBUG
    Serial.begin(115200);                       // For debug
    #endif
    /*                                           
    analogReference(INTERNAL);                  // Initialise internal reference for voltage monitor
    */

    setupDisplay();                             // Setup display
    refreshDisplay();                           // Refresh info on display
    setupTimer1();                              // Setup timer for emulator
}

void loop() {
    if (buttonTimer.isReady()) { button.tick(); }
    if (displayTimer.isReady()) { refreshDisplay(); }
    if (rfidOperationTimer.isReady()) { action(); }
    readButton();                               // Read buttons  
}