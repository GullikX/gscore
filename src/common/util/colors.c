/* Copyright (C) 2020-2024 Martin Gulliksson <martin@gullik.cc>
 *
 * This file is part of gscore.
 *
 * gscore is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 3.
 *
 * gscore is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>
 *
 */

#include "colors.h"

#include "common/util/hash.h"
#include "common/util/math.h"
#include "common/util/log.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static Color limitValues(Color color);


Color lerpColor(Color source, Color target, float lerpWeight) {
    Color color = {
        lerpWeight * target.r + (1.0 - lerpWeight) * source.r,
        lerpWeight * target.g + (1.0 - lerpWeight) * source.g,
        lerpWeight * target.b + (1.0 - lerpWeight) * source.b,
        lerpWeight * target.a + (1.0 - lerpWeight) * source.a,
    };

    return limitValues(color);
}


Color lerpColorDeltaTimeAdjusted(Color source, Color target, float lerpWeight, float deltaTime) {
    float deltaTimeAdjustedLerpWeight = Math_calculateDeltaTimeAdjustedLerpWeight(lerpWeight, deltaTime);
    Color color = lerpColor(source, target, deltaTimeAdjustedLerpWeight);

    return limitValues(color);
}


Color parseHexColor(const char* const hexColor) {
    int stringLength = strlen(hexColor);
    Log_assert(stringLength == 6, "The color string must contain exactly 6 hexadecimal digits (%d given)", stringLength);

    for (int iChar = 0; iChar < stringLength; iChar++) {
        Log_assert(isxdigit(hexColor[iChar]), "Character '%c' is not a hex digit", hexColor[iChar]);
    }

    long rgbColors[3];
    for (int iColorChannel = 0; iColorChannel < 3; iColorChannel++) {
        const char channelColor[] = {hexColor[2*iColorChannel], hexColor[2*iColorChannel + 1], '\n'};
        rgbColors[iColorChannel] = strtol(channelColor, NULL, 16);
    }

    Color color = {
        (float)rgbColors[0] / 255.0f,
        (float)rgbColors[1] / 255.0f,
        (float)rgbColors[2] / 255.0f,
        1.0,
    };

    return limitValues(color);
}

Color generateColorFromString(const char* const string) {
    enum {
        BUFFER_SIZE = 64,
    };

    char buffer[BUFFER_SIZE] = {0};

    size_t stringLength = strlen(string);
    size_t hash = hashDjb2(string, stringLength);

    snprintf(buffer, BUFFER_SIZE, "%zu", hash);
    size_t r = hashDjb2(buffer, BUFFER_SIZE);

    snprintf(buffer, BUFFER_SIZE, "%zu", r);
    size_t g = hashDjb2(buffer, BUFFER_SIZE);

    snprintf(buffer, BUFFER_SIZE, "%zu", g);
    size_t b = hashDjb2(buffer, BUFFER_SIZE);

    Color color = {(float)r / (float)SIZE_MAX, (float)g / (float)SIZE_MAX, (float)b / (float)SIZE_MAX, 1.0};

    return limitValues(color);
}


static Color limitValues(Color color) {
    return (Color) {
        Math_clampf(color.r, 0.0, 1.0),
        Math_clampf(color.g, 0.0, 1.0),
        Math_clampf(color.b, 0.0, 1.0),
        Math_clampf(color.a, 0.0, 1.0),
    };
}
