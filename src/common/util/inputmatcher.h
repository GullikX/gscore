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

#include "common/structs/charevent.h"
#include "common/structs/keyevent.h"
#include "common/structs/mousebuttonevent.h"

#include <stdbool.h>

bool charEventMatches(CharEvent* event, CharEvent* other);
bool keyEventMatches(KeyEvent* event, KeyEvent* other);
bool mouseButtonEventMatches(MouseButtonEvent* event, MouseButtonEvent* other);
