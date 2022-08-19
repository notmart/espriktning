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

#include <Adafruit_AHT10.h>

#include "SegmentPixels.h"
#include "Tokenizer.h"
#include "WifiMQTTManager.h"
#include "CommandLine.h"
#include "Settings.h"
#include "pm1006.h"

#define PIN_PM1006_RX  5 //D1
#define PIN_PM1006_TX  4 //D2
#define PIN_LDR        A0 //A0
#define PIN_FAN        13 //D7
#define PIN_PIXELS     14 //D5

static SoftwareSerial pmSerial(PIN_PM1006_RX, PIN_PM1006_TX);
static PM1006 pm1006(&pmSerial);

Adafruit_AHT10 aht;

static const uint fanOnTime = 0;
static const uint fanOffTime = 20000;
static const uint measurementTime = 30000;
static const uint displaySwitchTime = 10000;

unsigned long factoryResetButtonDownTime = 0;
unsigned long lastCycleTime = 0;
unsigned long lastDisplaySwitchTime = 0;

static const uint ldrInterval = 120000;

unsigned long lastLdrTime = 0;

bool ledsOn = true;
bool fan = false;


uint16_t pm2_5 = 0;

enum DisplayType {
    PM2_5 = 0,
    Temperature = 1,
    Humidity = 2
};

static DisplayType currentDisplay = PM2_5;

WifiMQTTManager wifiMQTT("ESPriktning");
NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> pixelBus(SegmentPixels::numPixelsForDigits(2,3), PIN_PIXELS);
SegmentPixels pixels(&pixelBus, 2, 3);
Tokenizer tokenizer;

void syncPixelsAnimation()
{
    for (int i = 0; i < 20; ++i) {
        pixels.updateAnimation();
        delay(25);
    }
}

void setup()
{
    enableWiFiAtBootTime();
    WiFi.setPhyMode(WIFI_PHY_MODE_11G);

    Serial.begin(115200);
    // Recycle the flash button as a factory reset
    pinMode(0, INPUT_PULLUP);
    pinMode(PIN_FAN, OUTPUT);
    pinMode(PIN_LDR, INPUT);

    pmSerial.begin(PM1006::BIT_RATE);

    Settings *s = Settings::self();
    s->load();

    lastCycleTime = millis();
    pixels.begin();
    pixels.setAnimationDuration(s->animationDuration());
    double intensity = double(s->ledIntensityAtDay()) / 100;
    pixels.setLedIntensity(intensity);

    ledsOn = intensity > 0;
    pixels.setColor(3,6,8);
    pixels.setNumber(88);
    syncPixelsAnimation();
 
    if (s->useWifi()) {
        wifiMQTT.setup();
        pixels.setColor(10,6,5);
        pixels.setNumber(88);
    }

    Wire.begin(0, 12);
    aht.begin();
}

void loop()
{
   Settings *s = Settings::self();

   // Every now and then shut down the leds, measure the light and shut down until is dark (configurable?)
   if (millis() - lastLdrTime > ldrInterval) {
       double intensity = double(s->ledIntensityAtNight()) / 100;
       pixels.setColor(0, 0, 0);
       ledsOn = false;
       if (millis() - lastLdrTime > ldrInterval + 2000) {
           Serial.print("LDR:");
           Serial.println(analogRead(PIN_LDR));
           double intensity = 0.0;
           if (analogRead(PIN_LDR) > 500) {
               intensity = double(s->ledIntensityAtDay()) / 100;
           } else {
               intensity = double(s->ledIntensityAtNight()) / 100;
           }
           ledsOn = intensity > 0;
           pixels.setLedIntensity(intensity);
           if (ledsOn) {
          //     pixels.setPM25ColorNumber(pm2_5);
           }
           lastLdrTime = millis();
       }
   }

    // TODO: parse some commands form the tokenizer
    if (tokenizer.tokenizeFromSerial()) {
        parseCommand(tokenizer, wifiMQTT, pixels);
    }

    // Commandline may have modified settings
    if (s->isDirty()) {
        s->save();
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

    if (millis() - lastDisplaySwitchTime > displaySwitchTime) {
        lastDisplaySwitchTime = millis();
        sensors_event_t humidity, temp;
        aht.getEvent(&humidity, &temp);
        Serial.print("Temperature: "); Serial.print(temp.temperature); Serial.println(" degrees C");
        Serial.print("Humidity: "); Serial.print(humidity.relative_humidity); Serial.println("% rH");
        if (ledsOn) {
            currentDisplay = DisplayType((currentDisplay + 1) % (Humidity + 1));
            switch (currentDisplay) {
            case PM2_5:
                Serial.println("showing PM2.5");
                pixels.setPM25ColorNumber(pm2_5);
                break;
            case Temperature:
                Serial.println("showing temp");
                pixels.setTempColorNumber(temp.temperature);
                break;
            case Humidity:
                Serial.println("showing humidity");
                pixels.setHumidityColorNumber(humidity.relative_humidity);
            }
        }
    }

    
    long delta = millis() - lastCycleTime;
    if (delta > measurementTime) {
        lastCycleTime = millis();

        printf("Attempting measurement:\n");
        if (pm1006.read_pm25(&pm2_5)) {
            Serial.print("New sensor value:");
            Serial.println(String(pm2_5).c_str());
        } else {
            Serial.println("Measurement failed");
        }

        // for some reason first time after startup the sensor reads a wrong value > 1000
        if (ledsOn && pm2_5 <= 1000) {
            //onst int num = round(double(pm2_5)/10);
          //  pixels.setPM25ColorNumber(pm2_5);
        }

        if (s->useWifi()) {
           // std::shared_ptr<PubSubClient> client = wifiMQTT.ensureMqttClientConnected();
           // client->loop();
            //client->publish(Settings::self()->mqttTopic(), String(pm2_5).c_str(), true);
            wifiMQTT.tryPublish(Settings::self()->mqttTopic(), String(pm2_5));
            sensors_event_t humidity, temp;
            aht.getEvent(&humidity, &temp);
            wifiMQTT.tryPublish("Temperature", String(temp.temperature));
            wifiMQTT.tryPublish("Humidity", String(humidity.relative_humidity));
        }
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
