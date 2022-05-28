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

#pragma once

#include <Arduino.h>

class Settings {
public:
    enum class Error {
        NoError = 0,
        SPIFFSError,
        JsonDeserializeError,
        FileNotFoundError,
        FileOpenError,
        FileWriteError
    };
 
    Settings();
    ~Settings();
 
    static Settings *self();

    void load();
    void save();

    bool isDirty() const;
    void printSettings() const;

    bool useWifi() const;
    void setUseWifi(bool useWifi);

    int ledIntensityAtDay() const;
    void setLedIntensityAtDay(int intensity);

    int ledIntensityAtNight() const;
    void setLedIntensityAtNight(int intensity);

    String mqttTopic() const;
    void setMqttTopic(const String &topic);

    String mqttServer() const;
    void setMqttServer(const String &server);

    //TODO: proper type
    String mqttPort() const;
    void setMqttPort(const String &port);

    String mqttUserName() const;
    void setMqttUserName(const String &userName);

    String mqttPassword() const;
    void setMqttPassword(const String &password);

private:
    static Settings *s_settings;

    bool m_useWifi = true;
    int m_ledIntensityAtDay = 100;
    int m_ledIntensityAtNight = 0;

    String m_mqttTopic;
    String m_mqttServer;
    String m_mqttPort;
    String m_mqttUserName;
    String m_mqttPassword;

    Error m_error = Error::NoError;
    bool m_dirty = true;
};
