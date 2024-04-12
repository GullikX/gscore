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

#include <stddef.h>

#define EVENT_CALLBACK(x) (void (*)(void*, void*, void*)) (x)

// Global event types
#define EVENT_APPLICATION_STATE_CHANGED "EVENT_APPLICATION_STATE_CHANGED"
#define EVENT_PROCESS_FRAME_BEGIN "EVENT_PROCESS_FRAME_BEGIN"
#define EVENT_PROCESS_FRAME "EVENT_PROCESS_FRAME"
#define EVENT_PROCESS_FRAME_END "EVENT_PROCESS_FRAME_END"
#define EVENT_CHAR_INPUT "EVENT_CHAR_INPUT"
#define EVENT_KEY_INPUT "EVENT_KEY_INPUT"
#define EVENT_MOUSE_BUTTON_INPUT "EVENT_MOUSE_BUTTON_INPUT"
#define EVENT_MOUSE_MOTION_INPUT "EVENT_MOUSE_MOTION_INPUT"
#define EVENT_MOUSE_SCROLLWHEEL_INPUT "EVENT_MOUSE_SCROLLWHEEL_INPUT"
#define EVENT_VIEWPORT_SIZE_UPDATED "EVENT_VIEWPORT_SIZE_UPDATED"
#define EVENT_NOTE_ADDED "EVENT_NOTE_ADDED"
#define EVENT_NOTE_REMOVED "EVENT_NOTE_REMOVED"
#define EVENT_BLOCK_INSTANCE_ADDED "EVENT_BLOCK_INSTANCE_ADDED"
#define EVENT_BLOCK_INSTANCE_REMOVED "EVENT_BLOCK_INSTANCE_REMOVED"
#define EVENT_ACTIVE_BLOCK_CHANGED "EVENT_ACTIVE_BLOCK_CHANGED"
#define EVENT_BLOCK_COLOR_CHANGED "EVENT_BLOCK_COLOR_CHANGED"
#define EVENT_KEY_SIGNATURE_CHANGED "EVENT_KEY_SIGNATURE_CHANGED"
#define EVENT_QUERY_RESULT "EVENT_QUERY_RESULT"
#define EVENT_REQUEST_CHANGE_SYNTH_INSTRUMENT "EVENT_REQUEST_CHANGE_SYNTH_INSTRUMENT"
#define EVENT_REQUEST_CHANGE_SYNTH_INSTRUMENT_QUERY "EVENT_REQUEST_CHANGE_SYNTH_INSTRUMENT_QUERY"
#define EVENT_REQUEST_DRAW_QUAD "EVENT_REQUEST_DRAW_QUAD"
#define EVENT_REQUEST_IGNORE_NOTEOFF "EVENT_REQUEST_IGNORE_NOTEOFF"
#define EVENT_REQUEST_MIDI_MESSAGE_PLAY "EVENT_REQUEST_MIDI_MESSAGE_PLAY"
#define EVENT_REQUEST_MIDI_CHANNEL_STOP "EVENT_REQUEST_MIDI_CHANNEL_STOP"
#define EVENT_REQUEST_QUERY "EVENT_REQUEST_QUERY"
#define EVENT_REQUEST_QUIT "EVENT_REQUEST_QUIT"
#define EVENT_REQUEST_SEQUENCER_START "EVENT_REQUEST_SEQUENCER_START"
#define EVENT_REQUEST_SEQUENCER_STOP "EVENT_REQUEST_SEQUENCER_STOP"
#define EVENT_SEQUENCER_STARTED "EVENT_SEQUENCER_STARTED"
#define EVENT_SEQUENCER_STOPPED "EVENT_SEQUENCER_STOPPED"
#define EVENT_SEQUENCER_PROGRESS "EVENT_SEQUENCER_PROGRESS"
#define EVENT_SYNTH_INSTRUMENT_CHANGED "EVENT_SYNTH_INSTRUMENT_CHANGED"

void Events_setup(void);
void Events_teardown(void);

void Events_defineNewLocalEventType(const char* eventName, size_t dataSize, void* receiver);

void Event_subscribe(const char* eventName, void* object, void (*callback)(void* self, void* sender, void* data), size_t dataSize);
void Event_unsubscribe(const char* eventName, void* object, void (*callback)(void* self, void* sender, void* data), size_t dataSize);
void Event_post(void* sender, const char* eventName, void* data, size_t dataSize);

