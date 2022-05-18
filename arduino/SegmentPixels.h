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
 
#include <Adafruit_NeoPixel.h>

class SegmentPixels {
public:
    SegmentPixels(int numDigits, int pin, int pixelType, int paddingPixels);
    ~SegmentPixels();

    void begin();
 
    void showNumber(int number);
    void setColor(int r, int g, int b);

private:
    static const int s_segmentFlags[10];

    Adafruit_NeoPixel m_pixels;
    int m_numPixels = 0;
    int m_numDigits = 0;
    int m_paddingPixels = 0;

    int m_number = 0;

    int m_red = 255;
    int m_green = 255;
    int m_blue = 255;
};
