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

#include "alloc.h"

#include "common/util/log.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static long nAllocations = 0;
static uintptr_t allocationPointerSum = 0;


void* ecalloc(size_t nItems, size_t itemSize) {
    void* pointer = calloc(nItems, itemSize);
    if (!pointer) {
        Log_fatal("ecalloc: Failed to allocate memory");
    }
    nAllocations++;
    allocationPointerSum += (uintptr_t)pointer;
    return pointer;
}


void* ememdup(const void* src, size_t nItems, size_t itemSize) {
    void* pointer = ecalloc(nItems, itemSize);
    memcpy(pointer, src, nItems * itemSize);
    return pointer;
}


char* estrdup(const char* const string) {
    char* stringDuped = strdup(string);
    if (!stringDuped) {
        Log_fatal("estrdup: Failed to allocate memory");
    }
    nAllocations++;
    allocationPointerSum += (uintptr_t)stringDuped;
    return stringDuped;
}


void sfree(void** pptr) {
    if (*pptr) {
        free(*pptr);
        nAllocations--;
        allocationPointerSum -= (uintptr_t)*pptr;
        *pptr = NULL;
    } else {
        Log_warning("sfree: null pointer provided");
    }
}


void printMemoryLeakWarning(void) {
    if (nAllocations != 0) {
        Log_warning("There are %ld memory allocations that have not been freed.", nAllocations);
    }
    if (allocationPointerSum != 0) {
        Log_warning("Inconsistency between allocated and freed memory pointers.");
    }
}
