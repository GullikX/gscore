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

#include "stringmap.h"

#include "common/util/alloc.h"
#include "common/util/hashmap.h"
#include "common/util/log.h"

#include <string.h>


struct StringMap {
    size_t keyBufferSize;
    char* keyBuffer;
    HashMap* hashMap;
};


static void StringMap_writeKeyToKeyBuffer(StringMap* self, const char* key);


StringMap* StringMap_new(size_t maxKeyStringLength) {
    StringMap* self = ecalloc(1, sizeof(*self));
    self->keyBufferSize = maxKeyStringLength + 1;
    self->keyBuffer = ecalloc(1, self->keyBufferSize);
    self->hashMap = HashMap_new(self->keyBufferSize);
    return self;
}


void StringMap_free(StringMap** pself) {
    StringMap* self = *pself;
    HashMap_free(&self->hashMap);
    sfree((void**)&self->keyBuffer);
    sfree((void**)pself);
}


bool StringMap_addItem(StringMap* self, const char* key, const void* value) {
    StringMap_writeKeyToKeyBuffer(self, key);
    return HashMap_addItem(self->hashMap, self->keyBuffer, value);
}


bool StringMap_containsItem(StringMap* self, const char* key) {
    StringMap_writeKeyToKeyBuffer(self, key);
    return HashMap_containsItem(self->hashMap, self->keyBuffer);
}


const void* StringMap_getItem(StringMap* self, const char* key) {
    StringMap_writeKeyToKeyBuffer(self, key);
    return HashMap_getItem(self->hashMap, self->keyBuffer);
}


bool StringMap_removeItem(StringMap* self, const char* key) {
    StringMap_writeKeyToKeyBuffer(self, key);
    return HashMap_removeItem(self->hashMap, self->keyBuffer);
}


const char* StringMap_iterateInit(StringMap* self) {
    return HashMap_iterateInit(self->hashMap);
}


const char* StringMap_iterateNext(StringMap* self, const char* keyPrevious) {
    StringMap_writeKeyToKeyBuffer(self, keyPrevious);
    return HashMap_iterateNext(self->hashMap, self->keyBuffer);
}


void StringMap_iterateBreak(StringMap* self) {
    HashMap_iterateBreak(self->hashMap);
}


size_t StringMap_countItems(StringMap* self) {
    return HashMap_countItems(self->hashMap);
}


void StringMap_clear(StringMap* self) {
    HashMap_clear(self->hashMap);
}


static void StringMap_writeKeyToKeyBuffer(StringMap* self, const char* key) {
    Log_assert(key, "null key");
    size_t keyLength = strlen(key);
    Log_assert(keyLength > 0 && keyLength < self->keyBufferSize, "invalid key length %zu", keyLength);
    memset(self->keyBuffer, 0, self->keyBufferSize);
    strncat(self->keyBuffer, key, self->keyBufferSize);
}
