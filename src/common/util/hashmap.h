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

typedef struct HashMap HashMap;

HashMap* HashMap_new(size_t keySizeBytes);
void HashMap_free(HashMap** pself);
bool HashMap_addItem(HashMap* self, const void* key, const void* value);
bool HashMap_containsItem(HashMap* self, const void* key);
const void* HashMap_getItem(HashMap* self, const void* key);
bool HashMap_removeItem(HashMap* self, const void* key);
const void* HashMap_iterateInit(HashMap* self);
const void* HashMap_iterateNext(HashMap* self, const void* keyPrevious);
void HashMap_iterateBreak(HashMap* self);
size_t HashMap_countItems(HashMap* self);
void HashMap_clear(HashMap* self);
