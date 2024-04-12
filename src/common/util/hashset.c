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

#include "hashset.h"

#include "common/util/alloc.h"
#include "common/util/hashmap.h"

struct HashSet {
    HashMap* hashMap;
};

HashSet* HashSet_new(size_t keySizeBytes) {
    HashSet* self = ecalloc(1, sizeof(*self));
    self->hashMap = HashMap_new(keySizeBytes);

    return self;
}


void HashSet_free(HashSet** pself) {
    HashSet* self = *pself;
    HashMap_free(&self->hashMap);
    sfree((void**)pself);
}


bool HashSet_addItem(HashSet* self, const void* key) {
    return HashMap_addItem(self->hashMap, key, NULL);
}


bool HashSet_containsItem(HashSet* self, const void* key) {
    return HashMap_containsItem(self->hashMap, key);
}


bool HashSet_removeItem(HashSet* self, const void* key) {
    return HashMap_removeItem(self->hashMap, key);
}


const void* HashSet_iterateInit(HashSet* self) {
    return HashMap_iterateInit(self->hashMap);
}


const void* HashSet_iterateNext(HashSet* self, const void* keyPrevious) {
    return HashMap_iterateNext(self->hashMap, keyPrevious);
}


void HashSet_iterateBreak(HashSet* self) {
    HashMap_iterateBreak(self->hashMap);
}


size_t HashSet_countItems(HashSet* self) {
    return HashMap_countItems(self->hashMap);
}


void HashSet_clear(HashSet* self) {
    HashMap_clear(self->hashMap);
}
