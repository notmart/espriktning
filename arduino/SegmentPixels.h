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

#include <NeoPixelBus.h>

class SegmentPixels {
public:
    // NeoPixelBus needs to be a global static in the man file, so pass at a pointer TODO: subclass?
    SegmentPixels(NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> *pixels, int numDigits, int paddingPixels);
    ~SegmentPixels();

    static int numPixelsForDigits(int numDigits, int paddingPixels);
    void begin();
 
    void setNumber(int number);
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setPM25ColorNumber(int number);
    void updateAnimation();

private:
    static const int s_segmentFlags[10];
    NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> *m_pixels;

   // Adafruit_NeoPixel m_pixels;
    int m_numPixels = 0;
    int m_numDigits = 0;
    int m_paddingPixels = 0;

    int m_number = 0;

    uint32_t m_pixelMask = 0;
    uint32_t m_oldPixelMask = 0;
    uint8_t m_red = 0;
    uint8_t m_green = 0;
    uint8_t m_blue = 0;

    uint8_t m_oldRed = 0;
    uint8_t m_oldGreen = 0;
    uint8_t m_oldBlue = 0;

    double m_animProgress = 0;
    unsigned long m_animStarted = 0;
    unsigned long m_lastFrame = 0;
    uint16_t m_animDuration = 1000;
};
