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

static void die(const char* const format, ...) {
    fputs("Error: ", stderr);
    va_list vaList;
    va_start(vaList, format);
    vfprintf(stderr, format, vaList);
    va_end(vaList);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}


static void warn(const char* const format, ...) {
    fputs("Warning: ", stderr);
    va_list vaList;
    va_start(vaList, format);
    vfprintf(stderr, format, vaList);
    va_end(vaList);
    fputc('\n', stderr);
}


static void* ecalloc(size_t nItems, size_t itemSize) {
    void* pointer = calloc(nItems, itemSize);
    if (!pointer) {
        die("Failed to allocate memory");
    }
    return pointer;
}


static bool fileExists(const char* const filename) {
    return access(filename, F_OK) != -1;
}


static void spawnSetXProp(int atomId) {
    int bufferLength = strlen(cmdQuery) + strlen(ATOM_PROMPTS[atomId]) + strlen(ATOM_NAMES[atomId]) + 64;
    char* cmd = ecalloc(bufferLength, sizeof(char));
    Window x11Window = Application_getInstance()->xevents->x11Window;
    snprintf(cmd, bufferLength, cmdQuery, x11Window, ATOM_PROMPTS[atomId], ATOM_NAMES[atomId]);
    const char* const pipeData = ATOM_FUNCTIONS[atomId]();
    spawn(cmd, pipeData);
}


static void spawn(const char* const cmd, const char* const pipeData) {
    FILE* pipe = popen(cmd, "w");
    if (!pipe) {
        die("Failed to run popen");
    }
    fprintf(pipe, "%s", pipeData);
    pclose(pipe);
}


static bool hexColorToRgb(const char* const hexColor, Vector4* rgbOut) {
    int stringLength = strlen(hexColor);
    if (stringLength != 6) {
        warn("The color string must contain exactly 6 hexadecimal digits (%d given)", stringLength);
        return false;
    }
    for (int iChar = 0; iChar < stringLength; iChar++) {
        if (!isxdigit(hexColor[iChar])) {
            warn("Character '%c' is not a hex digit", hexColor[iChar]);
            return false;
        }
    }

    long rgbColors[3];

    for (int iColorChannel = 0; iColorChannel < 3; iColorChannel++) {
        const char channelColor[] = {hexColor[2*iColorChannel], hexColor[2*iColorChannel + 1], '\n'};
        rgbColors[iColorChannel] = strtol(channelColor, NULL, 16);
    }

    rgbOut->x = (float)rgbColors[0] / 255.0f;
    rgbOut->y = (float)rgbColors[1] / 255.0f;
    rgbOut->z = (float)rgbColors[2] / 255.0f;
    rgbOut->w = 1.0f;

    return true;
}


static int modulo(int a, int b) {
    return (a % b + b) % b;
}
