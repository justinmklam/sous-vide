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

#define LCD_COLS 16     // LCD dimensions
#define LCD_ROWS 2      // LCD dimensions

#define I2C_ADDR    0x27 // <<----- Add your address here.  Find it from I2C Scanner
#define SDA_PIN 12       // I2C data pin
#define SCL_PIN 14      // I2C clock pin
#define RELAY_PIN 5
#define ENCODER_PIN_A 13
#define ENCODER_PIN_B 2
#define ENCODER_STEPS 4 // number of reading increments per detent
#define DEG_SYMBOL (char)223    // character code for degree symbol for LCD

#define TEMP_SENSOR_PIN 4  // DS18B20 pin

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature DS18B20(&oneWire);

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLS, LCD_ROWS);

Encoder myEnc(ENCODER_PIN_A, ENCODER_PIN_B);

float temp;
int relay_state = 0;
int32_t oldPosition  = -999;

void setup()
{
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN,LOW);

    Wire.begin(SDA_PIN, SCL_PIN);

    lcd.begin (LCD_COLS,LCD_ROWS); //  <<----- My LCD was 16xl2
    lcd.init();

    lcd.backlight();
    lcd.home (); // go home

    // lcd.print("ESP8266 SousVide");  
}

void loop()
{
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(0);

    // Construct char buffer to convert float to string
    char t[10];
    char buf[80];
    dtostrf(temp, 4, 1, t);
    sprintf(buf,"Temp: %s%cC",t, DEG_SYMBOL); 

    lcd.setCursor(0,1);
    lcd.print(buf);

    // digitalWrite(RELAY_PIN, (relay_state) ? HIGH:LOW);
    // relay_state = !relay_state;

    // Show encoder on LCD
    long newPosition = myEnc.read() / ENCODER_STEPS;
    if (newPosition != oldPosition) {
        oldPosition = newPosition;
        lcd.setCursor(4,0);
        lcd.print("                ");
        lcd.setCursor(0,0);
        sprintf(buf,"Set temp: %d%cC",newPosition, DEG_SYMBOL);
        lcd.print(buf);
        // lcd.print("Set temp: ");
        // lcd.print(newPosition);
    }

    // delay(1000);
}