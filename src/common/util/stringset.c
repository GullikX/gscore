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

#include "stringset.h"

#include "common/util/alloc.h"
#include "common/util/stringmap.h"

struct StringSet {
    StringMap* StringMap;
};

StringSet* StringSet_new(size_t maxKeyStringLength) {
    StringSet* self = ecalloc(1, sizeof(*self));
    self->StringMap = StringMap_new(maxKeyStringLength);

    return self;
}


void StringSet_free(StringSet** pself) {
    StringSet* self = *pself;
    StringMap_free(&self->StringMap);
    sfree((void**)pself);
}


bool StringSet_addItem(StringSet* self, const void* key) {
    return StringMap_addItem(self->StringMap, key, "NULL");
}


bool StringSet_containsItem(StringSet* self, const void* key) {
    return StringMap_containsItem(self->StringMap, key);
}


bool StringSet_removeItem(StringSet* self, const void* key) {
    return StringMap_removeItem(self->StringMap, key);
}


const void* StringSet_iterateInit(StringSet* self) {
    return StringMap_iterateInit(self->StringMap);
}


const void* StringSet_iterateNext(StringSet* self, const void* keyPrevious) {
    return StringMap_iterateNext(self->StringMap, keyPrevious);
}


void StringSet_iterateBreak(StringSet* self) {
    StringMap_iterateBreak(self->StringMap);
}


size_t StringSet_countItems(StringSet* self) {
    return StringMap_countItems(self->StringMap);
}


void StringSet_clear(StringSet* self) {
    StringMap_clear(self->StringMap);
}
