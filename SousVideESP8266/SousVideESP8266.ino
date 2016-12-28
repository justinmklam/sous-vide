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

#define I2C_ADDR    0x27 // <<----- Add your address here.  Find it from I2C Scanner
#define SDA_PIN 2
#define SCL_PIN 14

#define ONE_WIRE_BUS 4  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

LiquidCrystal_I2C lcd(I2C_ADDR, 16, 2);

float temp;

void setup()
{
    Wire.begin(SDA_PIN, SCL_PIN);

    lcd.begin (16,2); //  <<----- My LCD was 16xl2
    lcd.init();

    lcd.backlight();
    lcd.home (); // go home

    lcd.print("ESP8266 SousVide");  
}

void loop()
{
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(0);

    // Construct char buffer to convert float to string
    char t[10];
    char buf[80];
    dtostrf(temp, 4, 1, t);
    sprintf(buf,"Temp: %s%cC",t, (char)223);    // (char)223 is degree symbol

    lcd.setCursor(0,1);
    lcd.print(buf);

    lcd.setCursor(0,1);
    // lcd.print("Temp: ");
    // lcd.print(temp);
    // lcd.print((char)223);
    delay(2000);
}