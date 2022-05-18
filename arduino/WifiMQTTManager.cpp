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

WifiMQTTManager::WifiMQTTManager(char *captiveName, char *defaultTopic)
    : m_server(WiFiServer(80))
    , m_captiveName(captiveName)
{
    strcpy(m_mqttTopic, defaultTopic);
}

WifiMQTTManager::~WifiMQTTManager()
{}

void WifiMQTTManager::setup()
{
    WiFiManager wifiManager;
    WiFiManagerParameter mqttName("name", "Friendly Name", m_mqttTopic, 40);
    WiFiManagerParameter mqttServer("server", "MQTT Server", m_mqttServer, 40);
    WiFiManagerParameter mqttPort("port", "MQTT Port", m_mqttPort, 6);
    WiFiManagerParameter mqttUsername("username", "mqtt username", m_mqttUsername, 40);
    WiFiManagerParameter mqttPassword("password", "mqtt password", m_mqttPassword, 40);
    wifiManager.addParameter(&mqttName);
    wifiManager.addParameter(&mqttServer);
    wifiManager.addParameter(&mqttPort);
    wifiManager.addParameter(&mqttUsername);
    wifiManager.addParameter(&mqttPassword);

    wifiManager.setSaveConfigCallback([&]() {
        strcpy(m_mqttTopic, mqttName.getValue());
        strcpy(m_mqttServer, mqttServer.getValue());
        strcpy(m_mqttPort, mqttPort.getValue());
        strcpy(m_mqttUsername, mqttUsername.getValue());
        strcpy(m_mqttPassword, mqttPassword.getValue());
        saveMQTTConfig();
    });

    wifiManager.autoConnect(m_captiveName);
    m_status = Status(m_status | Status::WifiConnected);
    Serial.println("Connected.");

    readMQTTConfig();
}

void WifiMQTTManager::factoryReset()
{
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    SPIFFS.format();
    ESP.reset();
}

char *WifiMQTTManager::topic()
{
    return m_mqttTopic;
}

void WifiMQTTManager::saveMQTTConfig()
{
    Serial.println("WifiMQTTManager: saving config...");
    DynamicJsonDocument json(512);
    json["friendly_name"] = m_mqttTopic;
    json["mqtt_server"] = m_mqttServer;
    json["mqtt_port"] = m_mqttPort;
    json["mqtt_username"] = m_mqttUsername;
    json["mqtt_password"] = m_mqttPassword;

    if (SPIFFS.begin()) {
        Serial.println("WifiMQTTManager: mounted file system...");
        File configFile = SPIFFS.open("/config.json", "w");
        if (!configFile) {
            Serial.println("WifiMQTTManager: failed to open config file for writing...");
            m_error = Error::SPIFFSError;
            factoryReset();
            return;
        }
        serializeJson(json, configFile);
        serializeJsonPretty(json, Serial);
        Serial.println("");
        Serial.println("WifiMQTTManager: config.json saved.");
        configFile.close();
    } else {
        m_error = Error::SPIFFSError;
        Serial.println("WifiMQTTManager: failed to mount FS");
        Serial.println("WifiMQTTManager: formating FS...re-upload to try again...");
        factoryReset();
    }
}

void WifiMQTTManager::readMQTTConfig()
{
    Serial.println("WifiMQTTManager: mounting SPIFFS...");
    if (SPIFFS.begin()) {
        Serial.println("WifiMQTTManager: mounted file system...");
        if (SPIFFS.exists("/config.json")) {
            //file exists, reading and loading
            Serial.println("WifiMQTTManager: reading config file...");
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile) {
                Serial.println("WifiMQTTManager: opened config file...");
                size_t size = configFile.size();

                DynamicJsonDocument json(512);
                DeserializationError error = deserializeJson(json, configFile);
                if (error) {
                    m_error = Error::MQTTError;
                    Serial.print("WifiMQTTManager: failed to load json config: ");
                    Serial.println(error.c_str());
                } else {
                    Serial.println("\nWifiMQTTManager: parsed json...");
                    strcpy(m_mqttTopic, json["friendly_name"]);
                    strcpy(m_mqttServer, json["mqtt_server"]);
                    strcpy(m_mqttPort, json["mqtt_port"]);
                    strcpy(m_mqttUsername, json["mqtt_username"]);
                    strcpy(m_mqttPassword, json["mqtt_password"]);
                    m_status = Status(m_status | Status::MQTTHasCredentials);

                    Serial.println(m_mqttTopic);
                    Serial.println(m_mqttServer);
                    Serial.println(m_mqttPort);
                    Serial.println(m_mqttUsername);
                    Serial.println(m_mqttPassword);
                }
            } else {
                m_error = Error::MQTTError;
                Serial.println("WifiMQTTManager: cannot open config.json for reading, performing factory reset...");
                factoryReset();
            }
        } else {
            m_error = Error::MQTTError;
            Serial.println("WifiMQTTManager: could not find config file, performing factory reset...");
            factoryReset();
        }

    } else {
        m_error = Error::SPIFFSError;
        Serial.println("WifiMQTTManager: failed to mount FS");
        Serial.println("WifiMQTTManager: performing factory reset...");
        factoryReset();
    }
}

std::shared_ptr<PubSubClient> WifiMQTTManager::ensureMqttClientConnected()
{
    if (WiFi.status() != WL_CONNECTED) {
        setup();
    }
    if (!m_pubSubClient) {
        m_pubSubClient.reset(new PubSubClient(m_client));
        const unsigned short port = (unsigned short) strtoul(m_mqttPort, NULL, 0);
        m_pubSubClient->setServer(m_mqttServer, port);
    }
    while (!m_pubSubClient->connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (m_pubSubClient->connect(m_mqttTopic, m_mqttUsername, m_mqttPassword)) {
            Serial.println("MQTT connected");
        } else {
            Serial.print("failed:");
            Serial.print(m_pubSubClient->state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
    return m_pubSubClient;
}
