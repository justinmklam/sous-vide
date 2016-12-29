/*
Sous vide slow cooker controller on ESP8266.

Liquid Crystal Library: https://github.com/marcoschwartz/LiquidCrystal_I2C
Dallas temperature lib: https://github.com/milesburton/Arduino-Temperature-Control-Library
OneWire lib: https://github.com/PaulStoffregen/OneWire

Date: Tue, 27 Dec 2016
Author: Justin Lam
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Encoder.h>
#include "PushButton.h"

#define LCD_COLS 16     // LCD dimensions
#define LCD_ROWS 2      // LCD dimensions

#define I2C_ADDR    0x27 // <<----- Add your address here.  Find it from I2C Scanner
#define SDA_PIN 12       // I2C data pin
#define SCL_PIN 14      // I2C clock pin
#define BUTTON_PIN 0
#define RELAY_PIN 5
#define ENCODER_PIN_A 13
#define ENCODER_PIN_B 2
#define ENCODER_STEPS 4 // number of reading increments per detent
#define DEG_SYMBOL (char)223    // character code for degree symbol for LCD
#define SAMPLING_FREQ 100

#define TEMP_SENSOR_PIN 4  // DS18B20 pin

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature DS18B20(&oneWire);

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLS, LCD_ROWS);

Encoder myEnc(ENCODER_PIN_A, ENCODER_PIN_B);

bool buttonPressed = false;
bool modeChange = false;
uint8_t buttonState = 0;
uint8_t mode = 0;
uint8_t lastButtonState = 0;

int32_t defaultTemp = 80;   // starting temperature on first boot

float temp;
int relay_state = 0;
int32_t oldPosition  = -999;
// int32_t newPosition = 42;

uint32_t current_millis;
uint32_t previous_millis = 0;

uint32_t temp_offset = 80;
uint32_t time_offset = 0;

uint32_t set_time;
uint32_t set_temp;

char buf[40];

void setup() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN,LOW);

    Wire.begin(SDA_PIN, SCL_PIN);

    lcd.begin (LCD_COLS,LCD_ROWS); //  <<----- My LCD was 16xl2
    lcd.init();

    lcd.backlight();
    lcd.home (); // go home

    // lcd.print("ESP8266 SousVide");  

    attachInterrupt(BUTTON_PIN, buttonISR, FALLING);
}

void buttonISR() {
    static uint32_t last_interrupt_time = 0;
    uint32_t interrupt_time = millis();

    // If interrupts come faster than 200ms, assume it's a bounce and ignore
    if (interrupt_time - last_interrupt_time > 200) {
        buttonPressed = true;
    }
    last_interrupt_time = interrupt_time;
}

void loop() {
    if(buttonPressed) {
        lcd.clear();
        lcd.home();
        mode++;

        if(mode > 2) {
            mode = 0;
        }

        buttonPressed = false;
        modeChange = true;
    }

    // lcd.home();
    // lcd.print("Mode: ");
    // lcd.print(mode);

    current_millis = millis();

    if((current_millis - previous_millis ) >= SAMPLING_FREQ) {

        switch(mode) {
            case 1: {
                static int32_t old_time = 0;
                if(modeChange) {
                    time_offset = old_time - myEnc.read() / ENCODER_STEPS;
                    modeChange = false;
                }

                lcd.print("Set new time");
                set_time = myEnc.read() / ENCODER_STEPS + time_offset;

                // if (newPosition != oldPosition) {
                old_time = set_time;
                // lcd.setCursor(9,1);
                // lcd.print("                ");
                lcd.setCursor(0,1);
                sprintf(buf,"New time: %d hrs       ",set_time);
                lcd.print(buf);
            } break;

            case 2: {
                static int32_t old_temp = 80;

                if(modeChange) {
                    temp_offset = old_temp - myEnc.read() / ENCODER_STEPS;
                    modeChange = false;
                }

                lcd.print("Set new temp");
                set_temp = myEnc.read() / ENCODER_STEPS + temp_offset;

                // if (newPosition != oldPosition) {
                old_temp = set_temp;
                lcd.setCursor(9,1);
                // lcd.print("                ");
                lcd.setCursor(0,1);
                sprintf(buf,"New temp: %d%cC     ",set_temp, DEG_SYMBOL);
                lcd.print(buf);

                // if(buttonPressed) {
                //     temp_offset = newPosition;
                // }
                    // lcd.print("Set temp: ");
                    // lcd.print(newPosition);
                // }
            } break;

            default: {
                DS18B20.requestTemperatures(); 
                temp = DS18B20.getTempCByIndex(0);

                // Construct char buffer to convert float to string
                char t[10];
                dtostrf(temp, 4, 1, t);
                sprintf(buf,"T: %s%cC (%d%cC)     ",t, DEG_SYMBOL, set_temp, DEG_SYMBOL); 

                lcd.setCursor(0,0); 
                lcd.print(buf);

                // lcd.setCursor(0,0);
                // sprintf(buf,"T: 81.2%cC (80%cC)", DEG_SYMBOL, DEG_SYMBOL);
                // lcd.print(buf);
                lcd.setCursor(0,1);
                sprintf(buf,"t: 2h42 (%dh00)"      , set_time);
                lcd.print(buf);
            } break;
        }
        previous_millis = current_millis;
    }

}