/*
Sous vide slow cooker controller on ESP8266.

Liquid Crystal Library: https://github.com/marcoschwartz/LiquidCrystal_I2C
Dallas temperature lib: https://github.com/milesburton/Arduino-Temperature-Control-Library
OneWire lib: https://github.com/PaulStoffregen/OneWire
Adafruit_SSD1306: https://github.com/adafruit/Adafruit_SSD1306
Adafruit GFX: https://github.com/adafruit/Adafruit-GFX-Library

Date: Sat, 28 Jan 2017
Author: Justin Lam
*/

/***********************/
/****** LIBRARIES ******/
/***********************/
// Temperature libs
#include <OneWire.h>
#include <DallasTemperature.h>

// Misc libs
#include <Wire.h>

#include <Encoder.h>
#include "CountdownTimer.h"

// ESP8266 Wifi libs
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// Display libs
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <Fonts/FreeMono9pt7b.h>   

/*************************/
/****** GLOBAL VARS ******/
/*************************/
// Wifi access point
#define WLAN_SSID       "HouseOfLams-2.4G"
#define WLAN_PASS       "1amfami1y"

// Adafruit.io setup
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "justinmklam"
#define AIO_KEY         "e6023bd3c2354a79adeb6b8ea95223cd"

//Pin declarations
#define I2C_ADDR    0x3C // For Kuman 0.96" OLED I2C display.  Find it from I2C Scanner
#define SDA_PIN 12       // ESP8266 I2C data pin
#define SCL_PIN 14       // ESP8266 I2C clock pin
#define BUTTON_PIN 0
#define RELAY_PIN 4
#define TEMP_SENSOR_PIN 5  // DS18B20 pin
#define ENCODER_PIN_A 13
#define ENCODER_PIN_B 2

//Encoder params
#define ENCODER_STEPS_TEMP 4 // number of reading increments per detent
#define ENCODER_STEPS_TIME 0.16 // for 15 min increment
#define TIME_INCREMENT 15  // encoder time increment

// Display stuff
#define OLED_RESET 1  // tie to ESP8266 reset btn
#define DEG_SYMBOL (char)247    // degree symbol for OLED display

// Timing intervals
#define REFRESH_RATE_UI 100     // refresh the UI every 100 ms
#define MQTT_PUBLISH_FREQ 15000  // publish data to MQTT every 15 secs

// UI stuff
#define STARTING_TEMP 20
#define MAX_TEMP 100
#define MIN_TEMP 20

// Setup OLED display
Adafruit_SSD1306 display(OLED_RESET);

// Setup temp sensor
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature DS18B20(&oneWire);

// Setup encoder
Encoder myEnc(ENCODER_PIN_A, ENCODER_PIN_B);

// Setup timer
CountDownTimer timer;

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish tempMQTT = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/sous-vide");

bool pauseMode = true;
bool timerEnd = false;
bool buttonPressed = false;
bool modeChange = false;
uint8_t modeUI = 0;
bool backlightOn = true;

float temp_reading;
int relay_state = 0;

// Timing vars
uint32_t current_millis;
uint32_t previous_millis = 0;

// Vars to hold previous encoder vals
uint32_t temp_offset;
uint32_t time_offset;

// Startup vars
uint32_t set_temp = STARTING_TEMP;
int32_t raw_time = 0;
uint32_t set_hr = 0;
uint32_t set_min = 0;

// Buffer for printing to LCD
char buf[40];

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void setup() {
//    Serial.begin(115200);
//    delay(10);
//
    // Initialize LCD with I2C
    Wire.begin(SDA_PIN, SCL_PIN);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

    // Show adafruit splash screen
    display.display();
    delay(500);
    display.clearDisplay();

    // Initialize display
    display.setCursor(0,0);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.print("Connecting to wifi");

//    // Connect to WiFi access point.
//    Serial.println(); Serial.println();
//    Serial.print("Connecting to ");
//    Serial.println(WLAN_SSID);


    WiFi.begin(WLAN_SSID, WLAN_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(150);
        display.print(".");
//        Serial.print(".");
    }
    display.print("\nConnected!");
//    Serial.println();
//
//    Serial.println("WiFi connected");
//    Serial.println("IP address: "); Serial.println(WiFi.localIP());

    // Initialize pins
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN,HIGH);   // relay is switched off on HIGH

    attachInterrupt(BUTTON_PIN, buttonISR, FALLING);

    // Initialize timer
    timer.SetTimer(0,0,0);
    timer.StartTimer();
}

void buttonISR() {
    /* 
     *  Handles button press with debouncing using hardware interrupt
     */
    static uint32_t last_interrupt_time = 0;
    uint32_t interrupt_time = millis();
    
    // If interrupts come faster than 200ms, assume it's a bounce and ignore
    if (interrupt_time - last_interrupt_time > 200) {
        buttonPressed = true;
    }
    last_interrupt_time = interrupt_time;
}
 
uint16_t getTime(char type, float raw_time) {
    /*
     * Splits user inputted time to hours and minutes
     */
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

int32_t readEncoderTime() {
    /*
     * Read the encoder and return the scaled time
     */
    float reading;
    
    reading = myEnc.read() / ENCODER_STEPS_TIME;

    // Return the rounded multiple
    return ((reading + TIME_INCREMENT/2) / TIME_INCREMENT) * TIME_INCREMENT;
}

int32_t readEncoderTemp() {
    /*
     * Read the encoder and return the scaled temperature
     */
    return myEnc.read() / ENCODER_STEPS_TEMP;
}

float readTempSensor() {
    /*
     * Read the temperature sensor
     */
    DS18B20.requestTemperatures(); 
    return DS18B20.getTempCByIndex(0);
}

uint8_t checkScrollDir(float prev, float curr) {
    /*
     * Check encoder scroll direction. Returns true if fwd, false if reverse.
     */
     uint8_t dir;

     if( curr > prev ) {
        dir = 1;   // going forward
     }
     else if (prev > curr) {
        dir = 2;   // going backward
     }
     else {
        dir = 0;   // no change
     }
     
     return dir;
}

void setNewTime() {
    /*
     * Prompt user to set cooking time
     */
    static int32_t old_time = 0;

    // Reset to the previous user value
    if(modeChange) {
        time_offset = old_time - readEncoderTime();
        modeChange = false;
    }

    raw_time = readEncoderTime() + time_offset;
    
    if (raw_time < 0) {
      raw_time = 0;
    }
    old_time = raw_time;

    set_hr = getTime('h', raw_time);
    set_min = getTime('m', raw_time);

    if(set_hr == 0 && set_min == 0) {
        pauseMode = true;
    }
    else {
        pauseMode = false;
    }

    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Set time:\n");
    sprintf(buf," t = %dh%d               ",set_hr,set_min);
    display.println(buf);
    display.display();
}

void setNewTemp() {
    /*
     * Prompt user to set cooking temperature
     */
    static int32_t old_temp = STARTING_TEMP;

    // Reset to the previous user value
    if(modeChange) {
        temp_offset = old_temp - readEncoderTemp();
        modeChange = false;
    }
    
    set_temp = readEncoderTemp() + temp_offset;
    old_temp = set_temp;
    
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Set temp:\n");
    sprintf(buf," T = %d%cC               ",set_temp, DEG_SYMBOL);
    display.println(buf);
    display.display();
}

void monitorStatus() {
    /*
     * Show live cooking temperature and time status
     */
    temp_reading = readTempSensor();
    controlTemp(temp_reading, set_temp);

    // Send temperature reading online
    MQTT_publish(temp_reading);

    // Construct char buffer to convert float to string
    char t[10];
    dtostrf(temp_reading, 4, 1, t);

    // Show temperature status
    display.clearDisplay();
    if (temp_reading == -127){
      display.setCursor(7,0);
      display.setTextSize(2);
      display.println("  Insert");
      display.setCursor(7,18);
      display.println("   temp");
    }
    else {
      display.setTextSize(5);
      sprintf(buf,"%s",t, DEG_SYMBOL); 
      display.setCursor(7,0); 
      display.println(buf);
    }

    // Show time status
    display.setCursor(18,50); 
    display.setTextSize(2);
    sprintf(buf,"%02d:%02d:%02d       ",timer.ShowHours(),timer.ShowMinutes(),timer.ShowSeconds());
    display.println(buf);
    display.display();

    // Check if user wants to change time/temp settings
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
}

void controlTemp(float temp_reading, float desired_temp) {
    /*
     * "PID" temperature control
     */
    if (temp_reading < desired_temp) {
        digitalWrite(RELAY_PIN,LOW);
    }
    else {
        digitalWrite(RELAY_PIN,HIGH);
    }
}

void blinkLCD(uint8_t blink_rate) {
    /*
     * Flash the LCD backlight when timer is done
     */
    uint32_t current_millis = millis();
    static uint32_t previous_millis = 0;

    if ((current_millis - previous_millis) >= blink_rate) {
        if (backlightOn) {
            display.invertDisplay(true);
            backlightOn = false;
        }
        else {
            display.invertDisplay(false);
            backlightOn = true;
        }
        previous_millis = current_millis;
    }
}

void MQTT_publish(float val) {
    /*
     * Publish data through MQTT to Adafruit.io
     */
    uint32_t current_millis = millis();
    static uint32_t previous_millis = 0;

    if ((current_millis - previous_millis) >= MQTT_PUBLISH_FREQ) {
        MQTT_connect(); // reconnect if necessary

        if(!tempMQTT.publish(val)) {
            Serial.println(F("Failed"));
        }
        previous_millis = current_millis;
    }
}

void MQTT_connect() {
  /*
   * Connect to MQTT
   */
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}

void loop() {
    // Call this every clock cycle
    timerEnd = timer.RunTimer();

    if(!pauseMode && !timerEnd && modeUI == 0) {
        blinkLCD(100);
    }
    else if(!backlightOn) {
        display.invertDisplay(false);
//        display.backlight();
    }

    // Change UI mode if button is pressed
    if(buttonPressed) {
        display.clearDisplay();
        display.setCursor(0,0);
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
