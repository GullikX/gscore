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

typedef struct HashSet HashSet;

HashSet* HashSet_new(size_t keySizeBytes);
void HashSet_free(HashSet** pself);
bool HashSet_addItem(HashSet* self, const void* key);
bool HashSet_containsItem(HashSet* self, const void* key);
bool HashSet_removeItem(HashSet* self, const void* key);
const void* HashSet_iterateInit(HashSet* self);
const void* HashSet_iterateNext(HashSet* self, const void* keyPrevious);
void HashSet_iterateBreak(HashSet* self);
size_t HashSet_countItems(HashSet* self);
void HashSet_clear(HashSet* self);
