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

typedef struct StringSet StringSet;

StringSet* StringSet_new(size_t maxKeyStringLength);
void StringSet_free(StringSet** pself);
bool StringSet_addItem(StringSet* self, const void* key);
bool StringSet_containsItem(StringSet* self, const void* key);
bool StringSet_removeItem(StringSet* self, const void* key);
const void* StringSet_iterateInit(StringSet* self);
const void* StringSet_iterateNext(StringSet* self, const void* keyPrevious);
void StringSet_iterateBreak(StringSet* self);
size_t StringSet_countItems(StringSet* self);
void StringSet_clear(StringSet* self);
