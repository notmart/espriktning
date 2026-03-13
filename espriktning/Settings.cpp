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

#include "Settings.h"

#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include "FS.h" // SPIFFS is declared

Settings *Settings::s_settings = new Settings();

Settings::Settings()
    : m_pm2_5Topic("PM2_5")
    , m_temperatureTopic("Temperature")
    , m_humidityTopic("Humidity")
{}

Settings::~Settings()
{}

Settings *Settings::self()
{
    return Settings::s_settings;
}

bool Settings::isDirty() const
{
    return m_dirty;
}

void Settings::printSettings() const
{
    Serial.println("All available settings:");
    Serial.print("use_wifi:               ");
    Serial.println(m_useWifi ? "true" : "false");
    Serial.print("led_intensity_at_day:   ");
    Serial.println(m_ledIntensityAtDay);
    Serial.print("led_intensity_at_night: ");
    Serial.println(m_ledIntensityAtNight);
    Serial.print("animation_duration:     ");
    Serial.println(m_animationDuration);

    Serial.print("pm2_5_topic:             ");
    Serial.println(m_pm2_5Topic);
#if USE_AHT10
    Serial.print("temperature_topic:             ");
    Serial.println(m_temperatureTopic);
    Serial.print("humidity_topic:             ");
    Serial.println(m_humidityTopic);
#endif
    Serial.print("mqtt_server:            ");
    Serial.println(m_mqttServer);
    Serial.print("mqtt_port:              ");
    Serial.println(m_mqttPort);
    Serial.print("mqtt_user_name:         ");
    Serial.println(m_mqttUserName);
    Serial.print("mqtt_password:          ");
    Serial.println(m_mqttPassword);
}

void Settings::save()
{
    DynamicJsonDocument json(512);
    json["use_wifi"] = m_useWifi ? "true" : "false";
    json["animation_duration"] = m_animationDuration;
    json["led_intensity_at_day"] = m_ledIntensityAtDay;
    json["led_intensity_at_night"] = m_ledIntensityAtNight;

    json["pm2_5_topic"] = m_pm2_5Topic;
    json["temperature_topic"] = m_temperatureTopic;
    json["humidity_topic"] = m_humidityTopic;
    json["mqtt_server"] = m_mqttServer;
    json["mqtt_port"] = m_mqttPort;
    json["mqtt_username"] = m_mqttUserName;
    json["mqtt_password"] = m_mqttPassword;

    if (!SPIFFS.begin()) {
        // If fails first time, try to format
        SPIFFS.format();
    }

    if (!SPIFFS.begin()) {
        // Give up
        Serial.println("Settings: error saving config, failed to open SPIFFS");
        m_error = Error::SPIFFSError;
        return;
    }
 
    Serial.println("Settings: mounted file system...");
    File configFile = SPIFFS.open("/config.json", "w");

    if (!configFile) {
        Serial.println("Settings: failed to open config file for writing...");
        m_error = Error::FileWriteError;
        return;
    }
 
    serializeJson(json, configFile);
    serializeJsonPretty(json, Serial);
    Serial.println("");
    Serial.println("Settings: config.json saved.");
    configFile.close();
    m_dirty = false;
}

void Settings::load()
{
    Serial.println("Settings: mounting SPIFFS...");
    if (!SPIFFS.begin()) {
        m_error = Error::SPIFFSError;
        Serial.println("Settings: failed to mount FS");
        return;
    }

    Serial.println("Settings: mounted file system...");
    if (!SPIFFS.exists("/config.json")) {
        m_error = Error::FileNotFoundError;
        Serial.println("Settings: could not find config file, performing factory reset...");
        return;
    }

    //file exists, reading and loading
    Serial.println("Settings: reading config file...");
    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile) {
        m_error = Error::FileOpenError;
        Serial.println("Settings: cannot open config.json for reading");
        return;
    }

    size_t size = configFile.size();
    Serial.print("Settings: opened config file, size ");
    Serial.println(size);

    DynamicJsonDocument json(512);
    DeserializationError error = deserializeJson(json, configFile);
    if (error) {
        m_error = Error::JsonDeserializeError;
        Serial.print("Settings: failed to load json config: ");
        Serial.println(error.c_str());
        return;
    }

    if (json.containsKey("use_wifi")) {
        m_useWifi = json["use_wifi"] == "true";
    }
    if (json.containsKey("led_intensity_at_day")) {
        String numString(json["led_intensity_at_day"]);
        m_ledIntensityAtDay = min(long(100), max(long(0), numString.toInt()));
    }
    if (json.containsKey("led_intensity_at_night")) {
        String numString(json["led_intensity_at_night"]);
        m_ledIntensityAtNight = min(long(100), max(long(0), numString.toInt()));
    }
    if (json.containsKey("animation_duration")) {
        String numString(json["animation_duration"]);
        m_animationDuration = min(long(3000), max(long(0), numString.toInt()));
    }

    if (json.containsKey("pm2_5_topic")) {
        m_pm2_5Topic = String(json["pm2_5_topic"]);
    }
    if (json.containsKey("temperature_topic")) {
        m_temperatureTopic = String(json["temperature_topic"]);
    }
    if (json.containsKey("pm2_5_topic")) {
        m_humidityTopic = String(json["humidity_topic"]);
    }
    if (json.containsKey("mqtt_server")) {
        m_mqttServer = String(json["mqtt_server"]);
    }
    if (json.containsKey("mqtt_port")) {
        String numString(json["mqtt_port"]);
        m_mqttPort = max(long(0), min(long(65535), numString.toInt()));
    }
    if (json.containsKey("mqtt_username")) {
        m_mqttUserName = String(json["mqtt_username"]);
    }
    if (json.containsKey("mqtt_password")) {
        m_mqttPassword = String(json["mqtt_password"]);
    }

    Serial.println("\nSettings: parsed json...");
    serializeJsonPretty(json, Serial);
    Serial.println(m_animationDuration);
    m_dirty = false;
}

bool Settings::useWifi() const
{
    return m_useWifi;
}

void Settings::setUseWifi(bool useWifi)
{
    if (m_useWifi == useWifi) {
        return;
    }

    m_useWifi = useWifi;
    m_dirty = true;
}

uint16_t Settings::ledIntensityAtDay() const
{
    return m_ledIntensityAtDay;
}

void Settings::setLedIntensityAtDay(uint16_t intensity)
{
    if (m_ledIntensityAtDay == intensity) {
        return;
    }

    m_ledIntensityAtDay = intensity;
    m_dirty = true;
}

uint16_t Settings::ledIntensityAtNight() const
{
    return m_ledIntensityAtNight;
}

void Settings::setLedIntensityAtNight(uint16_t intensity)
{
    if (m_ledIntensityAtNight == intensity) {
        return;
    }

    m_ledIntensityAtNight = intensity;
    m_dirty = true;
}


uint16_t Settings::animationDuration() const
{
    return m_animationDuration;
}

void Settings::setAnimationDuration(uint16_t duration)
{
    if (m_animationDuration == duration) {
        return;
    }

    m_animationDuration = duration;
    m_dirty = true;
}

String Settings::pm2_5Topic() const
{
    return m_pm2_5Topic;
}

void Settings::setPm2_5Topic(const String &topic)
{
    if (m_pm2_5Topic == topic ) {
        return;
    }

    m_pm2_5Topic = topic;
    m_dirty = true;
}

String Settings::temperatureTopic() const
{
    return m_temperatureTopic;
}

void Settings::setTemperatureTopic(const String &topic)
{
    if (m_temperatureTopic == topic ) {
        return;
    }

    m_temperatureTopic = topic;
    m_dirty = true;
}

String Settings::humidityTopic() const
{
    return m_humidityTopic;
}

void Settings::setHumidityTopic(const String &topic)
{
    if (m_humidityTopic == topic ) {
        return;
    }

    m_humidityTopic = topic;
    m_dirty = true;
}

String Settings::mqttServer() const
{
    return m_mqttServer;
}

void Settings::setMqttServer(const String &server)
{
    if (m_mqttServer == server) {
        return;
    }

    m_mqttServer = server;
    m_dirty = true;
}

uint16_t Settings::mqttPort() const
{
    return m_mqttPort;
}

void Settings::setMqttPort(uint16_t port)
{
    if (m_mqttPort == port) {
        return;
    }

    m_mqttPort = port;
    m_dirty = true;
}

String Settings::mqttUserName() const
{
    return m_mqttUserName;
}

void Settings::setMqttUserName(const String &userName)
{
    if (m_mqttUserName == userName) {
        return;
    }

    m_mqttUserName = userName;
    m_dirty = true;
}

String Settings::mqttPassword() const
{
    return m_mqttPassword;
}

void Settings::setMqttPassword(const String &password)
{
    if (m_mqttPassword == password) {
        return;
    }

    m_mqttPassword = password;
    m_dirty = true;
}
