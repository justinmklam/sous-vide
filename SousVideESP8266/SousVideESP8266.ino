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
#include "CountdownTimer.h"

#define LCD_COLS 16     // LCD dimensions
#define LCD_ROWS 2      // LCD dimensions

#define I2C_ADDR    0x27 // <<----- Add your address here.  Find it from I2C Scanner
#define SDA_PIN 12       // I2C data pin
#define SCL_PIN 14      // I2C clock pin
#define BUTTON_PIN 0
#define RELAY_PIN 5
#define ENCODER_PIN_A 13
#define ENCODER_PIN_B 2
#define ENCODER_STEPS_TEMP 4 // number of reading increments per detent
#define ENCODER_STEPS_TIME 0.16
#define DEG_SYMBOL (char)223    // character code for degree symbol for LCD
#define REFRESH_RATE_UI 100     // refresh the UI every 100 ms
#define STARTING_TEMP 65
#define MAX_TEMP 100
#define MIN_TEMP 20

#define TEMP_SENSOR_PIN 4  // DS18B20 pin

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature DS18B20(&oneWire);

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLS, LCD_ROWS);

Encoder myEnc(ENCODER_PIN_A, ENCODER_PIN_B);

CountDownTimer timer;

bool pauseMode = false;
bool timerEnd = false;
bool buttonPressed = false;
bool modeChange = false;
// uint8_t buttonState = 0;
uint8_t modeUI = 0;
// uint8_t lastButtonState = 0;
bool backlightOn = true;

// int32_t defaultTemp = 80;   // starting temperature on first boot

float temp_reading;
int relay_state = 0;
// int32_t oldPosition  = -999;
// int32_t newPosition = 42;

uint32_t current_millis;
uint32_t previous_millis = 0;

uint32_t temp_offset;
uint32_t time_offset;

uint32_t raw_time = 0;
uint32_t set_temp = STARTING_TEMP;

uint32_t set_hr = 0;
uint32_t set_min = 0;

// Buffer for printing to LCD
char buf[40];

void setup() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN,HIGH);   // relay is switched off on HIGH

    // Initialize LCD with I2C
    Wire.begin(SDA_PIN, SCL_PIN);
    lcd.begin (LCD_COLS,LCD_ROWS); 
    lcd.init();

    lcd.backlight();
    lcd.home (); // go home

    // lcd.print("ESP8266 SousVide");  

    attachInterrupt(BUTTON_PIN, buttonISR, FALLING);

    timer.SetTimer(0,0,3);
    timer.StartTimer();
}

// Handles button press with debouncing
void buttonISR() {
    static uint32_t last_interrupt_time = 0;
    uint32_t interrupt_time = millis();

    // If interrupts come faster than 200ms, assume it's a bounce and ignore
    if (interrupt_time - last_interrupt_time > 200) {
        buttonPressed = true;
    }
    last_interrupt_time = interrupt_time;
}

// Splits user inputted time to hours and minutes
uint16_t getTime(char type, float raw_time) {
    uint16_t hr, min;

    raw_time /= 100;
    hr = raw_time;      // Extract hours

    min = (raw_time - hr) * 100 * 0.6;  // Extract minutes in 15 min increments

    if(type == 'h') {
        return hr;
    }
    else if (type == 'm') {
        return min;
    }
}

// Read the encoder and return the scaled time
int32_t readEncoderTime() {
    return myEnc.read() / ENCODER_STEPS_TIME;
}

// Read the encoder and return the scaled temperature
int32_t readEncoderTemp() {
    return myEnc.read() / ENCODER_STEPS_TEMP;
}

// Read the temperature sensor
float readTempSensor() {
    DS18B20.requestTemperatures(); 
    return DS18B20.getTempCByIndex(0);
}

// Prompt user to set cooking time
void setNewTime() {
    static int32_t old_time = 0;

    // Reset to the previous user value
    if(modeChange) {
        time_offset = old_time - readEncoderTime();
        modeChange = false;
    }

    raw_time = readEncoderTime() + time_offset;
    old_time = raw_time;

    set_hr = getTime('h', raw_time);
    set_min = getTime('m', raw_time);

    if(set_hr == 0 && set_min == 0) {
        pauseMode = true;
    }
    else {
        pauseMode = false;
    }

    lcd.print("Set new time");
    lcd.setCursor(0,1);
    sprintf(buf,"New time: %dh%d       ",set_hr,set_min);
    lcd.print(buf);
}

// Prompt user to set cooking temperature
void setNewTemp() {
    static int32_t old_temp = STARTING_TEMP;

    // Reset to the previous user value
    if(modeChange) {
        temp_offset = old_temp - readEncoderTemp();
        modeChange = false;
    }
    
    set_temp = readEncoderTemp() + temp_offset;
    old_temp = set_temp;

    lcd.print("Set new temp");
    lcd.setCursor(0,1);
    sprintf(buf,"New temp: %d%cC     ",set_temp, DEG_SYMBOL);
    lcd.print(buf);
}

// Show live cooking temperature and time status
void monitorStatus() {
    temp_reading = readTempSensor();

    controlTemp(temp_reading, set_temp);

    // Construct char buffer to convert float to string
    char t[10];
    dtostrf(temp_reading, 4, 1, t);

    // Show temperature status
    sprintf(buf,"T| %s%cC (%d%cC)     ",t, DEG_SYMBOL, set_temp, DEG_SYMBOL); 
    lcd.setCursor(0,0); 
    lcd.print(buf);

    if(modeChange) {
        static uint32_t prev_hr = 999, prev_min = 999;

        // Reinitialize the timer if it's been changed
        if(set_hr != prev_hr || set_min != prev_min) {
            timer.SetTimer(set_hr, set_min, 0);
            timer.StartTimer();

            prev_hr = set_hr;
            prev_min = set_min;
        }
        modeChange = false;
    }

    // Show time status
    sprintf(buf,"t| %d:%d:%d       ",timer.ShowHours(),timer.ShowMinutes(),timer.ShowSeconds());
    lcd.setCursor(0,1);
    lcd.print(buf);
}

void controlTemp(float temp_reading, float desired_temp) {
    if (temp_reading < desired_temp) {
        digitalWrite(RELAY_PIN,LOW);
    }
    else {
        digitalWrite(RELAY_PIN,HIGH);
    }
}

void blinkLCD(uint8_t blink_rate) {
    static uint32_t current_millis = millis();
    static uint32_t previous_millis = 0;

    if ((current_millis - previous_millis) >= blink_rate) {
        if (backlightOn) {
            lcd.noBacklight();
            backlightOn = false;
        }
        else {
            lcd.backlight();
            backlightOn = true;
        }
    }
}

void loop() {
        // Call this every clock cycle
    timerEnd = timer.RunTimer();
    // timer.StopTimerAt(0,0,0);

    if(!pauseMode && !timerEnd && modeUI == 0) {
        blinkLCD(500);
    }
    else if(!backlightOn) {
        lcd.backlight();
    }

    // Change UI mode if button is pressed
    if(buttonPressed) {
        lcd.clear();
        lcd.home();
        modeUI++;

        // Cycle through the modes
        if(modeUI > 2) {
            modeUI = 0;
        }

        buttonPressed = false;
        modeChange = true;
    }

    current_millis = millis();

    // UI doesn't need to be updated as frequently
    if((current_millis - previous_millis ) >= REFRESH_RATE_UI) {
        switch(modeUI) { 
            // Set time
            case 1: { 
                setNewTime();
            } break;

            // Set temperature
            case 2: {
                setNewTemp();
            } break;

            // Live status
            default: {
                monitorStatus();
            } break;
        }
        previous_millis = current_millis;
    }

}