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

#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

enum {
    LOGLEVEL_INFO,
    LOGLEVEL_WARNING,
    LOGLEVEL_ERROR,
    LOGLEVEL_FATAL,
    LOGLEVEL_COUNT,
};

static const char* const LOGLEVEL_IDENTIFIERS = "IWEF";

static void print(int loglevel, const char* const format, va_list vaList);


void Log_info(const char* const format, ...) {
    va_list vaList;
    va_start(vaList, format);
    print(LOGLEVEL_INFO, format, vaList);
    va_end(vaList);
}


void Log_warning(const char* const format, ...) {
    va_list vaList;
    va_start(vaList, format);
    print(LOGLEVEL_WARNING, format, vaList);
    va_end(vaList);
}


void Log_error(const char* const format, ...) {
    va_list vaList;
    va_start(vaList, format);
    print(LOGLEVEL_ERROR, format, vaList);
    va_end(vaList);
}


void Log_fatal(const char* const format, ...) {
    va_list vaList;
    va_start(vaList, format);
    print(LOGLEVEL_FATAL, format, vaList);
    va_end(vaList);
    abort();
}


void Log_assert(bool condition, const char* const format, ...) {
    if (!condition) {
        va_list vaList;
        va_start(vaList, format);
        print(LOGLEVEL_FATAL, format, vaList);
        va_end(vaList);
        abort();
    }
}


static void print(int loglevel, const char* const format, va_list vaList) {
    char buffer[64];
    time_t current_time = time(NULL);
    struct tm* tm = localtime(&current_time);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);

    printf("%s  %c  ", buffer, LOGLEVEL_IDENTIFIERS[loglevel]);
    vprintf(format, vaList);
    printf("\n");
}
