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

enum {
    N_NOTES_IN_OCTAVE = 12,
};

typedef enum {
    C_MAJOR,

    G_MAJOR,
    D_MAJOR,
    A_MAJOR,
    E_MAJOR,
    B_MAJOR,
    F_SHARP_MAJOR,
    C_SHARP_MAJOR,

    F_MAJOR,
    B_FLAT_MAJOR,
    E_FLAT_MAJOR,
    A_FLAT_MAJOR,
    D_FLAT_MAJOR,
    G_FLAT_MAJOR,
    C_FLAT_MAJOR,

    KEY_SIGNATURE_COUNT,
} KeySignature;

static const int KEY_SIGNATURES[KEY_SIGNATURE_COUNT][N_NOTES_IN_OCTAVE] = {
    // C   C#  D   D#  E   F   F#  G   G#  A   A#  B
    {  1,  0,  1,  0,  1,  1,  0,  1,  0,  1,  0,  1},

    {  1,  0,  1,  0,  1,  0,  1,  1,  0,  1,  0,  1},
    {  0,  1,  1,  0,  1,  0,  1,  1,  0,  1,  0,  1},
    {  0,  1,  1,  0,  1,  0,  1,  0,  1,  1,  0,  1},
    {  0,  1,  0,  1,  1,  0,  1,  0,  1,  1,  0,  1},
    {  0,  1,  0,  1,  1,  0,  1,  0,  1,  0,  1,  1},
    {  0,  1,  0,  1,  0,  1,  1,  0,  1,  0,  1,  1},
    {  1,  1,  0,  1,  0,  1,  1,  0,  1,  0,  1,  0},

    {  1,  0,  1,  0,  1,  1,  0,  1,  0,  1,  1,  0},
    {  1,  0,  1,  1,  0,  1,  0,  1,  0,  1,  1,  0},
    {  1,  0,  1,  1,  0,  1,  0,  1,  1,  0,  1,  0},
    {  1,  1,  0,  1,  0,  1,  0,  1,  1,  0,  1,  0},
    {  1,  1,  0,  1,  0,  1,  1,  0,  1,  0,  1,  0},
    {  0,  1,  0,  1,  0,  1,  1,  0,  1,  0,  1,  1},
    {  0,  1,  0,  1,  1,  0,  1,  0,  1,  0,  1,  1},
};

static KeySignature KEY_SIGNATURE_DEFAULT = C_MAJOR;

static const char* KEY_SIGNATURE_NAMES[KEY_SIGNATURE_COUNT] = {
    "C major / A minor",

    "G major / E minor",
    "D major / B minor",
    "A major / F-sharp minor",
    "E major / C-sharp minor",
    "B major / G-sharp minor",
    "F-sharp major / D-sharp minor",
    "C-sharp major / A-sharp minor",

    "F major / D minor",
    "B-flat major / G minor",
    "E-flat major / C minor",
    "A-flat major / F minor",
    "D-flat major / B-flat minor",
    "G-flat major / E-flat minor",
    "C-flat major / A-flat minor",
};

static const char* const KEY_SIGNATURE_LIST_STRING =
    "C major / A minor\n"

    "G major / E minor\n"
    "D major / B minor\n"
    "A major / F-sharp minor\n"
    "E major / C-sharp minor\n"
    "B major / G-sharp minor\n"
    "F-sharp major / D-sharp minor\n"
    "C-sharp major / A-sharp minor\n"

    "F major / D minor\n"
    "B-flat major / G minor\n"
    "E-flat major / C minor\n"
    "A-flat major / F minor\n"
    "D-flat major / B-flat minor\n"
    "G-flat major / E-flat minor\n"
    "C-flat major / A-flat minor\n";

static const char* NOTE_NAMES[N_NOTES_IN_OCTAVE] = {
    "C",
    "C#",
    "D",
    "D#",
    "E",
    "F",
    "F#",
    "G",
    "G#",
    "A",
    "A#",
    "B",
};
