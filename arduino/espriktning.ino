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

static const uint fanOnTime = 5000;
static const uint fanOffTime = 8000;
static const uint measurementTime = 10000;

bool fan = false;

int num = 0;

WifiMQTTManager wifiMQTT("ESPriktning", "PM2.5");
SegmentPixels pixels(2, PIN_PIXELS, NEO_GRB, 3);
Tokenizer tokenizer;

void setup()
{
    Serial.begin(115200);
    // Recycle the flash button as a factory reset
    pinMode(0, INPUT_PULLUP);
  //  wifiMQTT.setup();
    pmSerial.begin(PM1006::BIT_RATE);
    lastMsg = millis();
    pinMode(PIN_FAN, OUTPUT);
    pinMode(PIN_LDR, INPUT);
    pixels.begin();
}

void loop() {

   // TODO: every now and then shut down the leds, measure the light and shut down until is dark (configurable?)
   // Serial.print("LDR:");
   // Serial.println(analogRead(PIN_LDR));

   //TODO: delete, just to test number encoding to leds
   //pixels.setColor(8, 2, 0);
   //pixels.showNumber(num++);
   //if (num > 99) num = 0;
   //delay(500);return;

    // TODO: parse some commands form the tokenizer
    if (tokenizer.tokenizeFromSerial()) {
        Serial.print("Tokens: ");
        Serial.println(tokenizer.numTokens());
        for (int i = 0; i < tokenizer.numTokens(); ++i) {
            Serial.print(i);
            Serial.print(": ");
            Serial.println(tokenizer[i]);
        }
    }
 

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

        pixels.showNumber(pm2_5);
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
}
