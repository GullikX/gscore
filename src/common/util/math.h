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

#pragma once

int Math_abs(int value);
float Math_clampf(float value, float min, float max);
int Math_clampi(int value, int min, int max);
int Math_max(int a, int b);
float Math_maxf(float a, float b);
int Math_min(int a, int b);
float Math_minf(float a, float b);
int Math_posmod(int a, int b);
float Math_powf(float base, float exponent);
float Math_calculateDeltaTimeAdjustedLerpWeight(float lerpWeight, float deltaTime);
