#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <GyverButton.h>
#include <EEPROM.h>

// Nokia display - Adafruit_PCD8544(CLK,DIN,D/C,CE,RST);
#define LCD_CLK A4
#define LCD_DIN A3
#define LCD_DC A2
#define LCD_CE A1
#define LCD_RST A0

#define BUTTON_PIN 10

// GButton button(BUTTON_PIN);
Adafruit_PCD8544 display = Adafruit_PCD8544(LCD_CLK, LCD_DIN, LCD_DC, LCD_CE, LCD_RST);

void setup() {
    
    // button.setTickMode(AUTO);
        //Initialize Display
    display.begin();

    // you can change the contrast around to adapt the display for the best viewing!
    display.setContrast(30);

    // Clear the buffer.
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(BLACK);
    display.setCursor(0,0);
    display.println("Hello world!");
    display.display();
    // delay(2000);
    // display.clearDisplay();
}


void loop() {

}

