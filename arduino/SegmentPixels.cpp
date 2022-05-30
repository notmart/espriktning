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
 
#include "SegmentPixels.h"

#include <math.h>

 const int SegmentPixels::s_segmentFlags[10] = {
        0b1110111, // 0
        0b0100100, // 1
        0b1011101, // 2
        0b1101101, // 3
        0b0101110, // 4
        0b1101011, // 5
        0b1111011, // 6
        0b0100101, // 7
        0b1111111, // 8
        0b1101111  // 9
    };

SegmentPixels::SegmentPixels(NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> *pixels, int numDigits, int paddingPixels)
    : m_numPixels(numPixelsForDigits(numDigits, paddingPixels))
    , m_pixels(pixels)
    , m_numDigits(numDigits)
    , m_paddingPixels(paddingPixels)
{
}

SegmentPixels::~SegmentPixels()
{
}

int SegmentPixels::numPixelsForDigits(int numDigits, int paddingPixels)
{
    return numDigits * 7 + paddingPixels;
}

void SegmentPixels::begin()
{
    m_pixels->Begin();
    setNumber(m_number);
}

void SegmentPixels::updateAnimation()
{
    unsigned long absTime = millis();
    // 25 msecs corresponds to ~40 fps
    if (absTime - m_lastFrame <= 25 || absTime - m_animStarted > m_animDuration) {
        return;
    }

    // TODO: easing curves
    m_animProgress = min(1.0, double(absTime - m_animStarted) / m_animDuration);
    m_lastFrame = absTime;

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    bool inOldMask = false;
    bool inNewMask = false;
    double dim = 0.0;
 
    for (uint16_t i = 0; i < m_numPixels; ++i) {
        inOldMask = m_oldPixelMask & (1 << i);
        inNewMask = m_pixelMask & (1 << i);
 
        if (inOldMask && inNewMask) {
            dim = 1.0;
        } else if (inNewMask) {
            dim = m_animProgress;
        } else if (inOldMask) {
            dim = 1.0 - m_animProgress;
        } else {
            dim = 0.0;
        }

        r = min(256.0, max(0.0, m_oldRed * (1 - m_animProgress) + (m_red * m_animProgress))) * dim;
        g = min(256.0, max(0.0, m_oldGreen * (1 - m_animProgress) + (m_green * m_animProgress))) * dim;
        b = min(256.0, max(0.0, m_oldBlue * (1 - m_animProgress) + (m_blue * m_animProgress))) * dim;
        m_pixels->SetPixelColor(i, RgbColor(r, g, b));
    }

    m_pixels->Show();
}

void SegmentPixels::setNumber(int number)
{
    if (number == m_number) {
        return;
    }
    m_number = number;
    m_oldPixelMask = m_pixelMask;
    m_pixelMask = 0;

    int numDigits = 0;
    int temp = number;
    while (temp > 0) {
        temp /= 10;
        ++numDigits;
    }

    if (numDigits == 0) {
        return;
    }

    int numberArray[numDigits];

    int count = 0;
    temp = number;

    while (temp != 0) {
        numberArray[numDigits - count - 1] = temp % 10;
        temp /= 10;
        count++;
    }
 
    int start = 0;
    for (int i = 0; i < m_paddingPixels; ++i) {
        m_pixelMask |= 1 << i;
        ++start;
    }


    for (int i = 0; i < m_numDigits - numDigits; ++i) {
        for (int j = 0; j < 7; ++j) {
            start++;
            //m_pixelMask |= 1 << start++;
        }
    }
    for (int i = max(0, numDigits - m_numDigits); i < numDigits; ++i) {
        m_pixelMask |= s_segmentFlags[numberArray[i]] << start;
        start += 7;
    }
    m_animStarted = millis();
}


void SegmentPixels::setColor(uint8_t r, uint8_t g, uint8_t b)
{
    if (r == m_red && g == m_green && b == m_blue) {
        return;
    }
    m_oldRed = m_red;
    m_oldGreen = m_green;
    m_oldBlue = m_blue;

    m_red = r;
    m_green = g;
    m_blue = b;
    m_animStarted = millis();
}

void SegmentPixels::setPM25ColorNumber(int number)
{
    const int num = max(0, min(99, number));
    setColor(
        max(0, (num - 20) / 2),
        max(0, 40 - num),
        num <= 15 ? 15 - num : 0);
    setNumber(num);
}
