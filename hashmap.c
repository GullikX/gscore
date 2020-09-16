/* Copyright (C) 2020 Martin Gulliksson <martin@gullik.cc>
 *
 * This file is part of gscore.
 *
 * gscore is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, version 3.
 *
 * gscore is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>
 *
 */

static HashMap* HashMap_new(int nBuckets) {
    HashMap* self = ecalloc(1, sizeof(*self));
    self->nBuckets = nBuckets;
    self->buckets = ecalloc(nBuckets, sizeof(HashMapEntry*));

    return self;
}


static HashMap* HashMap_free(HashMap* self) {
    for (int iBucket = 0; iBucket < self->nBuckets; iBucket++) {
        HashMapEntry* entry = self->buckets[iBucket];
        while(entry) {
            HashMapEntry* entryTemp = entry;
            entry = entry->next;
            HashMapEntry_free(entryTemp);
        }
    }
    free(self->buckets);
    free(self);
    return NULL;
}


static void HashMap_put(HashMap* self, const char* key, int value) {
    int iBucket = modulo(HashMap_djb2(key), self->nBuckets);
    if (!self->buckets[iBucket]) {
        self->buckets[iBucket] = HashMapEntry_new(key, value);
    }
    else {
        HashMapEntry* entry = self->buckets[iBucket];
        for (; entry->next; entry = entry->next);
        entry->next = HashMapEntry_new(key, value);
    }
}


static int HashMap_get(HashMap* self, const char* key) {
    int iBucket = modulo(HashMap_djb2(key), self->nBuckets);
    for (HashMapEntry* entry = self->buckets[iBucket]; entry; entry = entry->next) {
        if (!strcmp(entry->key, key)) {
            return entry->value;
        }
    }
    printf("Error: Did not find map entry for '%s'\n", key);
    return -1;
}


static int HashMap_djb2(const char* str) {
    int hash = 5381;
    int c;
    while (c = *str++) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}


static HashMapEntry* HashMapEntry_new(const char* key, int value) {
    HashMapEntry* self = ecalloc(1, sizeof(*self));
    self->key = strdup(key);
    self->value = value;
    self->next = NULL;
    return self;
}


static HashMapEntry* HashMapEntry_free(HashMapEntry* self) {
    if (self) {
        free(self->key);
        free(self);
    }
    return NULL;
}
