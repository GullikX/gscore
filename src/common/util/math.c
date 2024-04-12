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

#include "math.h"

#include <math.h>


int Math_abs(int value) {
    return (value > 0) ? value : -value;
}


float Math_clampf(float value, float min, float max) {
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    } else {
        return value;
    }
}


int Math_clampi(int value, int min, int max) {
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    } else {
        return value;
    }
}


int Math_max(int a, int b) {
    return (a > b) ? a : b;
}


float Math_maxf(float a, float b) {
    return (a > b) ? a : b;
}


int Math_min(int a, int b) {
    return (a < b) ? a : b;
}


float Math_minf(float a, float b) {
    return (a < b) ? a : b;
}


int Math_posmod(int a, int b) {
    return (a % b + b) % b;
}


float Math_powf(float base, float exponent) {
    return powf(base, exponent);
}


float Math_calculateDeltaTimeAdjustedLerpWeight(float lerpWeight, float deltaTime) {
    return 1.0 - powf(10.0, -lerpWeight * deltaTime);
}
