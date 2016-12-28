/*
Example Arduino sketch for SainSmart I2C LCD Screen 16x2.

Based on: https://bitbucket.org/celem/sainsmart-i2c-lcd/src/3adf8e0d2443/sainlcdtest.ino
Liquid Crystal Library: https://github.com/marcoschwartz/LiquidCrystal_I2C
Tested on ESP8266 module.

Date: Tue, 27 Dec 2016
Author: Justin Lam
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define I2C_ADDR    0x27 // <<----- Add your address here.  Find it from I2C Scanner
#define SDA_PIN 2
#define SCL_PIN 14

LiquidCrystal_I2C lcd(I2C_ADDR, 16, 2);

void setup()
{
Wire.begin(SDA_PIN, SCL_PIN);

lcd.begin (16,2); //  <<----- My LCD was 16x2
lcd.init();

lcd.backlight();
lcd.home (); // go home

lcd.print("Hello world!");  
}

void loop()
{
}