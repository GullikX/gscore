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

extern const int MIDI_MESSAGE_TYPE_NOTEOFF;
extern const int MIDI_MESSAGE_TYPE_NOTEON;

enum {
    MIDI_MESSAGE_VELOCITY_MIN = 0,
    MIDI_MESSAGE_VELOCITY_MAX = 127,
    MIDI_MESSAGE_VELOCITY_COUNT = MIDI_MESSAGE_VELOCITY_MAX - MIDI_MESSAGE_VELOCITY_MIN + 1,
};

enum {
    MIDI_MESSAGE_PITCH_MIN = 0,
    MIDI_MESSAGE_PITCH_MAX = 127,
    MIDI_MESSAGE_PITCH_COUNT = MIDI_MESSAGE_PITCH_MAX - MIDI_MESSAGE_PITCH_MIN + 1,
};

enum {
    MIDI_SYNTH_PROGRAM_NAME_LENGTH_MAX = 20,
};
