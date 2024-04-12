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

#include "hashmap.h"

#include "common/util/alloc.h"
#include "common/util/hash.h"
#include "common/util/log.h"
#include "common/util/math.h"

#include <string.h>

enum {
    N_BUCKETS = 4096,
};


typedef struct HashMapEntry HashMapEntry;
struct HashMapEntry {
    void* key;
    const void* value;
    HashMapEntry* nextInBucket;
    HashMapEntry* prevInBucket;
    HashMapEntry* nextInMap;
    HashMapEntry* prevInMap;
};


struct HashMap {
    HashMapEntry** buckets;
    HashMapEntry* entryFirst;
    HashMapEntry* entryLast;
    size_t keySizeBytes;
    size_t nItems;
    bool isIterating;
};


static size_t HashMap_hashKey(HashMap* self, const void* key);
static HashMapEntry* HashMapEntry_new(size_t keySizeBytes, const void* key, const void* value);
static void HashMapEntry_free(HashMapEntry** pself);
static bool HashMapEntry_updateValue(HashMapEntry* self, const void* value);


HashMap* HashMap_new(size_t keySizeBytes) {
    HashMap* self = ecalloc(1, sizeof(*self));
    self->buckets = ecalloc(N_BUCKETS, sizeof(HashMapEntry*));
    self->entryFirst = NULL;
    self->entryLast = NULL;
    self->keySizeBytes = keySizeBytes;
    self->nItems = 0;

    return self;
}


void HashMap_free(HashMap** pself) {
    HashMap* self = *pself;
    HashMap_clear(self);
    sfree((void**)&self->buckets);
    sfree((void**)pself);
}


bool HashMap_addItem(HashMap* self, const void* key, const void* value) {
    Log_assert(!self->isIterating, "Cannot add item to map while it is being iterated!");

    size_t iBucket = HashMap_hashKey(self, key);
    if (!self->buckets[iBucket]) {
        HashMapEntry* entryNew = HashMapEntry_new(self->keySizeBytes, key, value);
        self->buckets[iBucket] = entryNew;
        if (self->entryLast) {
            self->entryLast->nextInMap = entryNew;
        }
        entryNew->prevInMap = self->entryLast;
        self->entryLast = entryNew;
        if (!self->entryFirst) {
            self->entryFirst = entryNew;
        }
        self->nItems++;
        return true;
    }
    else {
        for (HashMapEntry* entry = self->buckets[iBucket]; entry; entry = entry->nextInBucket) {
            if (!memcmp(key, entry->key, self->keySizeBytes)) {
                return HashMapEntry_updateValue(entry, value);
            }
        }

        HashMapEntry* entry = self->buckets[iBucket];
        for (; entry->nextInBucket; entry = entry->nextInBucket);

        HashMapEntry* entryNew = HashMapEntry_new(self->keySizeBytes, key, value);
        entry->nextInBucket = entryNew;
        entryNew->prevInBucket = entry;
        if (self->entryLast) {
            self->entryLast->nextInMap = entryNew;
        }
        entryNew->prevInMap = self->entryLast;
        self->entryLast = entryNew;
        if (!self->entryFirst) {
            self->entryFirst = entryNew;
        }
        self->nItems++;
        return true;
    }
}


bool HashMap_containsItem(HashMap* self, const void* key) {
    size_t iBucket = HashMap_hashKey(self, key);
    for (HashMapEntry* entry = self->buckets[iBucket]; entry; entry = entry->nextInBucket) {
        if (!memcmp(key, entry->key, self->keySizeBytes)) {
            return true;
        }
    }
    return false;
}


const void* HashMap_getItem(HashMap* self, const void* key) {
    size_t iBucket = HashMap_hashKey(self, key);
    for (HashMapEntry* entry = self->buckets[iBucket]; entry; entry = entry->nextInBucket) {
        if (!memcmp(key, entry->key, self->keySizeBytes)) {
            return entry->value;
        }
    }
    Log_fatal("Did not find map entry for '%p' (key size: %zu bytes)", key, self->keySizeBytes);
    return NULL;
}


bool HashMap_removeItem(HashMap* self, const void* key) {
    Log_assert(!self->isIterating, "Cannot remove item from map while it is being iterated!");

    size_t iBucket = HashMap_hashKey(self, key);
    for (HashMapEntry* entry = self->buckets[iBucket]; entry; entry = entry->nextInBucket) {
        if (!memcmp(key, entry->key, self->keySizeBytes)) {
            if (entry->nextInBucket) {
                entry->nextInBucket->prevInBucket = entry->prevInBucket;
            }
            if (entry->prevInBucket) {
                entry->prevInBucket->nextInBucket = entry->nextInBucket;
            } else {
                Log_assert(entry == self->buckets[iBucket], "HashMap internal inconsistency");
                self->buckets[iBucket] = entry->nextInBucket;
            }

            if (entry == self->entryFirst) {
                self->entryFirst = entry->nextInMap;
            }
            if (entry == self->entryLast) {
                self->entryLast = entry->prevInMap;
            }
            if (entry->nextInMap) {
                entry->nextInMap->prevInMap = entry->prevInMap;
            }
            if (entry->prevInMap) {
                entry->prevInMap->nextInMap = entry->nextInMap;
            }

            HashMapEntry_free(&entry);
            self->nItems--;
            return true;
        }
    }
    Log_warning("Cannot remove non-existing key '%p' from map", key);
    return false;
}


const void* HashMap_iterateInit(HashMap* self) {
    Log_assert(!self->isIterating, "Already iterating!");

    if (self->entryFirst) {
        self->isIterating = true;
        return self->entryFirst->key;
    } else {
        return NULL;
    }
}


const void* HashMap_iterateNext(HashMap* self, const void* keyPrevious) {
    Log_assert(self->isIterating, "Must call iterateInit first!");

    size_t iBucket = HashMap_hashKey(self, keyPrevious);
    for (HashMapEntry* entry = self->buckets[iBucket]; entry; entry = entry->nextInBucket) {
        if (!memcmp(keyPrevious, entry->key, self->keySizeBytes)) {
            HashMapEntry* entryNextInMap = entry->nextInMap;
            if (entryNextInMap) {
                return entryNextInMap->key;
            } else {
                self->isIterating = false;
                return NULL;
            }
        }
    }

    Log_fatal("Did not find map entry for '%p' (key size: %zu bytes)", keyPrevious, self->keySizeBytes);
    return NULL;
}


void HashMap_iterateBreak(HashMap* self) {
    self->isIterating = false;
}


size_t HashMap_countItems(HashMap* self) {
    return self->nItems;
}


void HashMap_clear(HashMap* self) {
    Log_assert(!self->isIterating, "Cannot clear map while it is being iterated!");

    HashMapEntry* entry = self->entryFirst;
    while (entry) {
        if (entry->prevInBucket) {
            entry->prevInBucket->nextInBucket = entry->nextInBucket;
        } else {
            size_t iBucket = HashMap_hashKey(self, entry->key);
            Log_assert(entry == self->buckets[iBucket], "HashMap internal inconsistency");
            self->buckets[iBucket] = entry->nextInBucket;
        }
        if (entry->nextInBucket) {
            entry->nextInBucket->prevInBucket = entry->prevInBucket;
        }

        HashMapEntry* entryTemp = entry;
        entry = entry->nextInMap;
        HashMapEntry_free(&entryTemp);
        self->nItems--;
    }
    Log_assert(self->nItems == 0, "HashMap not empty after clear");
    self->entryFirst = NULL;
    self->entryLast = NULL;
}


static size_t HashMap_hashKey(HashMap* self, const void* key) {
    return hashDjb2(key, self->keySizeBytes) % N_BUCKETS;
}


static HashMapEntry* HashMapEntry_new(size_t keySizeBytes, const void* key, const void* value) {
    HashMapEntry* self = ecalloc(1, sizeof(*self));
    self->key = ecalloc(1, keySizeBytes);
    memcpy(self->key, key, keySizeBytes);
    self->value = value;
    self->nextInBucket = NULL;
    self->nextInMap = NULL;
    self->prevInMap = NULL;
    return self;
}


static void HashMapEntry_free(HashMapEntry** pself) {
    HashMapEntry* self = *pself;
    sfree((void**)&self->key);
    sfree((void**)pself);
}


static bool HashMapEntry_updateValue(HashMapEntry* self, const void* value) {
    if (self->value != value) {
        self->value = value;
        return true;
    }
    return false;
}
