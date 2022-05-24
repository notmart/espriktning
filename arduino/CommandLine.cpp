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
#include "CommandLine.h"

void showHelp()
{
    Serial.println("Commands:");
    Serial.println("help            Shows this help");
    Serial.println("factoryreset    forgets wifi and other settings, starts as new");
}

void parseCommand(Tokenizer &tokenizer, WifiMQTTManager &manager)
{
    if (tokenizer.numTokens() == 1) {
        if (tokenizer[0] == "help") {
            showHelp();
        } else if  (tokenizer[0] == "factoryreset") {
            manager.factoryReset();
        }
    }
}
