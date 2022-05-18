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
        0b0010010, // 1
        0b1011101, // 2
        0b1011011, // 3
        0b0111010, // 4
        0b1101011, // 5
        0b1101111, // 6
        0b1010010, // 7
        0b1111111, // 8
        0b1111011  // 9
    };

SegmentPixels::SegmentPixels(int numDigits, int pin, int pixelType, int paddingPixels)
    : m_numPixels(numDigits * 7 + paddingPixels)
    , m_pixels(Adafruit_NeoPixel(m_numPixels, pin, pixelType))
    , m_numDigits(numDigits)
    , m_paddingPixels(paddingPixels)
{
}

SegmentPixels::~SegmentPixels()
{
}

void SegmentPixels::begin()
{
    m_pixels.begin();
}


// TODO: to refactor and separe computation of bitmask vs actual coloring of pixels
void SegmentPixels::showNumber(int number)
{
    m_number = number;

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
        numberArray[numDigits-count-1] = temp % 10;
        temp /= 10;
        count++;
    }

    int start = 0;
    for (int i = 0; i < m_paddingPixels; ++i) {
        m_pixels.setPixelColor(i, m_pixels.Color(m_red, m_green, m_blue));
        ++start;
    }

    for (int i = 0; i < m_numDigits - numDigits; ++i) {
        for (int j = 0; j < 7; ++j) {
            m_pixels.setPixelColor(start++, m_pixels.Color(0, 0, 0));
        }
    }
    for (int i = max(0, numDigits - m_numDigits); i < numDigits; ++i) {
        for (int j = 0; j < 7; ++j) {
            if (s_segmentFlags[numberArray[i]] & (1 << (6-j))) {
                m_pixels.setPixelColor(start++, m_pixels.Color(m_red, m_green, m_blue));
            } else {
                m_pixels.setPixelColor(start++, m_pixels.Color(0, 0, 0));
            }
        }
    }
    m_pixels.show();
}

void SegmentPixels::setColor(int r, int g, int b)
{
    m_red = r;
    m_green = g;
    m_blue = b;
   // showNumber(m_number);
}
