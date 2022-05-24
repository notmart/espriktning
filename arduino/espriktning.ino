/*
 *   Copyright 2022 Marco Martin <notmart@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
 
#include <Arduino.h>
#include <SoftwareSerial.h>

#include "SegmentPixels.h"
#include "Tokenizer.h"
#include "WifiMQTTManager.h"
#include "CommandLine.h"
#include "pm1006.h"

#define PIN_PM1006_RX  5 //D1
#define PIN_PM1006_TX  4 //D2
#define PIN_LDR        A0 //A0
#define PIN_FAN        13 //D7
#define PIN_PIXELS     14 //D5

static SoftwareSerial pmSerial(PIN_PM1006_RX, PIN_PM1006_TX);
static PM1006 pm1006(&pmSerial);

unsigned long factoryResetButtonDownTime = 0;

unsigned long lastMsg = 0;

static const uint fanOnTime = 0;
static const uint fanOffTime = 20000;
static const uint measurementTime = 30000;

static const uint ldrInterval = 120000;
unsigned long lastLdrTime = 0;
bool ledsOn = true;

bool fan = false;

unsigned long lastNumUp = 0;
int num = 0;

WifiMQTTManager wifiMQTT("ESPriktning", "PM2.5");
NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> pixelBus(SegmentPixels::numPixelsForDigits(2,3), PIN_PIXELS);
SegmentPixels pixels(&pixelBus, 2, 3);
Tokenizer tokenizer;

void setup()
{
    Serial.begin(115200);
    // Recycle the flash button as a factory reset
    pinMode(0, INPUT_PULLUP);
    pinMode(PIN_FAN, OUTPUT);
    pinMode(PIN_LDR, INPUT);

    pmSerial.begin(PM1006::BIT_RATE);
    lastMsg = millis();
    pixels.begin();
    pixels.setColor(3,6,8);
    pixels.setNumber(100);
   // wifiMQTT.setup();
}

void loop() {

   // TODO: every now and then shut down the leds, measure the light and shut down until is dark (configurable?)
   // Serial.print("LDR:");
   // Serial.println(analogRead(PIN_LDR));
   if (millis() - lastLdrTime > ldrInterval) {
       pixels.setColor(0, 0, 0);
       pixels.setNumber(0);
       ledsOn = false;
       if (millis() - lastLdrTime > ldrInterval + 2000) {
           Serial.print("LDR:");
           Serial.println(analogRead(PIN_LDR));
           ledsOn = analogRead(PIN_LDR) > 500;
           lastLdrTime = millis();
       }
   }

   //TODO: delete, just to test number encoding to leds
   /*unsigned long absTime = millis();
   if (absTime - lastNumUp > 1000) {
       pixels.setColor(max(0, (num-100)/20), max(0, 25-(num-20)/20), num <= 100 ? 10 - num/10 : 0);
       pixels.setNumber(num/10);
       //Serial.println(num);

       lastNumUp = absTime;

       //delay(500);return;
   } else {
       pixels.updateAnimation();
   }*/



    // TODO: parse some commands form the tokenizer
    if (tokenizer.tokenizeFromSerial()) {
       /*Serial.print("Tokens: ");
        Serial.println(tokenizer.numTokens());
        for (int i = 0; i < tokenizer.numTokens(); ++i) {
            Serial.print(i);
            Serial.print(": ");
            Serial.println(tokenizer[i]);
        }*/
        //num = tokenizer[0].toInt();
        parseCommand(tokenizer, wifiMQTT);
    }

    // flash/factory reset button
    if (digitalRead(0) == 0) {
        if (factoryResetButtonDownTime == 0) {
            factoryResetButtonDownTime = millis();
        } else if (millis() - factoryResetButtonDownTime > 2000) {
            wifiMQTT.factoryReset();
        }
    } else {
        factoryResetButtonDownTime = 0;
    }

    long delta = millis() - lastMsg;
    if (delta > measurementTime) {
        lastMsg = millis();

        printf("Attempting measurement:\n");
        uint16_t pm2_5;
        if (pm1006.read_pm25(&pm2_5)) {
            printf("PM2.5 = %u\n", pm2_5);
        } else {
            printf("Measurement failed!\n");
        }

        Serial.print("New sensor value:");
        Serial.println(String(pm2_5).c_str());

        // for some reason on first startup the sensor reads a wrong value > 1000
        if (ledsOn && pm2_5 <= 1000) {
            int num = pm2_5/10;
            pixels.setColor(
                max(0, (num - 100) / 20),
                max(0, 25 - (num - 20) / 20),
                num <= 15 ? 15 - num : 0);
            pixels.setNumber(num);
        }
        
    //   std::shared_ptr<PubSubClient> client = wifiMQTT.ensureMqttClientConnected();
    //   client->loop();
    //   client->publish(wifiMQTT.topic(), String(pm2_5).c_str(), true);
    } else if (delta > fanOffTime) {
        if (fan) {
            Serial.println("turning fan off");
            digitalWrite(PIN_FAN, 0);
            fan = false;
        }
    } else if (delta > fanOnTime) {
        if (!fan) {
            Serial.println("turning fan on");
            digitalWrite(PIN_FAN, 1);
            fan = true;
        }
    }
    pixels.updateAnimation();
}
