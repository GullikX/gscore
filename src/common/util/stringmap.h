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

#include <stdbool.h>
#include <stddef.h>

typedef struct StringMap StringMap;

StringMap* StringMap_new(size_t maxKeyStringLength);
void StringMap_free(StringMap** pself);
bool StringMap_addItem(StringMap* self, const char* key, const void* value);
bool StringMap_containsItem(StringMap* self, const char* key);
const void* StringMap_getItem(StringMap* self, const char* key);
bool StringMap_removeItem(StringMap* self, const char* key);
const char* StringMap_iterateInit(StringMap* self);
const char* StringMap_iterateNext(StringMap* self, const char* keyPrevious);
void StringMap_iterateBreak(StringMap* self);
size_t StringMap_countItems(StringMap* self);
void StringMap_clear(StringMap* self);
