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
 
#include "WifiMQTTManager.h"
#include "Settings.h"

WifiMQTTManager::WifiMQTTManager(char *captiveName)
    : m_server(WiFiServer(80))
    , m_captiveName(captiveName)
{
}

WifiMQTTManager::~WifiMQTTManager()
{}

void WifiMQTTManager::setup()
{
    WiFiManager wifiManager;
    Settings *s = Settings::self();
    WiFiManagerParameter mqttName("name", "Friendly Name", s->mqttTopic().c_str(), 40);
    WiFiManagerParameter mqttServer("server", "MQTT Server", s->mqttServer().c_str(), 40);
    WiFiManagerParameter mqttPort("port", "MQTT Port", s->mqttPort().c_str(), 6);
    WiFiManagerParameter mqttUserName("username", "mqtt username", s->mqttUserName().c_str(), 40);
    WiFiManagerParameter mqttPassword("password", "mqtt password", s->mqttPassword().c_str(), 40);
    wifiManager.addParameter(&mqttName);
    wifiManager.addParameter(&mqttServer);
    wifiManager.addParameter(&mqttPort);
    wifiManager.addParameter(&mqttUserName);
    wifiManager.addParameter(&mqttPassword);

    wifiManager.setSaveConfigCallback([&]() {
        Settings *s = Settings::self();
        s->setMqttTopic(mqttName.getValue());
        s->setMqttServer(mqttServer.getValue());
        s->setMqttPort(mqttPort.getValue());
        s->setMqttUserName(mqttUserName.getValue());
        s->setMqttPassword(mqttPassword.getValue());
        s->save();
    });

    wifiManager.autoConnect(m_captiveName);
    m_status = Status(m_status | Status::WifiConnected);
    Serial.println("Connected.");

   // readMQTTConfig();
}

void WifiMQTTManager::factoryReset()
{
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    SPIFFS.format();
    ESP.reset();
}

bool WifiMQTTManager::tryPublish(const String &topic, const String &val)
{
    if (WiFi.status() != WL_CONNECTED) {
        setup();
    }

    Settings *s = Settings::self();

    if (!m_pubSubClient) {
        m_pubSubClient.reset(new PubSubClient(m_client));
        const unsigned short port = (unsigned short)s->mqttPort().toInt();
        m_pubSubClient->setServer(s->mqttServer().c_str(), port);
    }

    int attempts = 0;

    while (attempts < 5 || !m_pubSubClient->connected()) {
        ++attempts;
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (m_pubSubClient->connect(topic.c_str(), s->mqttUserName().c_str(), s->mqttPassword().c_str())) {
            Serial.println("MQTT connected");
        } else {
            Serial.print("failed:");
            Serial.print(m_pubSubClient->state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }

    if (m_pubSubClient->connected()) {
        m_pubSubClient->loop();
        m_pubSubClient->publish(topic.c_str(), val.c_str(), true);
        return true;
    } else {
        Serial.println("Server not responding: giving up");
        return false;
    }
}
