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

#include "events.h"

#include "common/constants/applicationstate.h"
#include "common/structs/blockinstance.h"
#include "common/structs/charevent.h"
#include "common/structs/keyevent.h"
#include "common/structs/midimessage.h"
#include "common/structs/mousebuttonevent.h"
#include "common/structs/note.h"
#include "common/structs/quad.h"
#include "common/structs/queryrequest.h"
#include "common/structs/queryresult.h"
#include "common/structs/vector2.h"
#include "common/structs/vector2i.h"
#include "common/structs/sequencerrequest.h"
#include "common/structs/synthprogramchange.h"
#include "common/util/alloc.h"
#include "common/util/hashset.h"
#include "common/util/log.h"
#include "common/util/stringmap.h"

#include <string.h>

enum {
    EVENT_NAME_MAX_LENGTH = 63,
};

#pragma pack(push, 1)
typedef struct EventSubscriber EventSubscriber;
struct EventSubscriber {
    void* object;
    void (*callback)(void* self, void* sender, void* data);
};
#pragma pack(pop)

typedef struct Event Event;
struct Event {
    size_t dataSize;
    void* receiver;
    HashSet* subscribers; // EventSubscriber
};

typedef struct Events Events;
struct Events {
    StringMap* eventMap; // str -> Event
};


static Events* instance = NULL;

static void Events_defineNewGlobalEventType(const char* eventName, size_t dataSize);
static void Events_defineNewEventTypeInternal(const char* eventName, size_t dataSize, void* receiver);


void Events_setup(void) {
    Log_assert(!instance, "events instance already created");

    instance = ecalloc(1, sizeof(*instance));
    instance->eventMap = StringMap_new(EVENT_NAME_MAX_LENGTH);

    typedef struct EventTypeInitializer EventTypeInitializer;
    struct EventTypeInitializer {
        const char* name;
        size_t dataSize;
    };

    EventTypeInitializer defaultEventTypes[] = {
        {EVENT_APPLICATION_STATE_CHANGED, sizeof(ApplicationState)},
        {EVENT_PROCESS_FRAME_BEGIN, sizeof(float)},
        {EVENT_PROCESS_FRAME, sizeof(float)},
        {EVENT_PROCESS_FRAME_END, sizeof(float)},
        {EVENT_CHAR_INPUT, sizeof(CharEvent)},
        {EVENT_KEY_INPUT, sizeof(KeyEvent)},
        {EVENT_MOUSE_BUTTON_INPUT, sizeof(MouseButtonEvent)},
        {EVENT_MOUSE_MOTION_INPUT, sizeof(Vector2)},
        {EVENT_MOUSE_SCROLLWHEEL_INPUT, sizeof(Vector2i)},
        {EVENT_VIEWPORT_SIZE_UPDATED, sizeof(Vector2i)},
        {EVENT_NOTE_ADDED, sizeof(Note)},
        {EVENT_NOTE_REMOVED, sizeof(Note)},
        {EVENT_BLOCK_INSTANCE_ADDED, sizeof(BlockInstance)},
        {EVENT_BLOCK_INSTANCE_REMOVED, sizeof(BlockInstance)},
        {EVENT_ACTIVE_BLOCK_CHANGED, sizeof(Color)},
        {EVENT_BLOCK_COLOR_CHANGED, sizeof(Color)},
        {EVENT_KEY_SIGNATURE_CHANGED, sizeof(int)},
        {EVENT_QUERY_RESULT, sizeof(QueryResult)},
        {EVENT_REQUEST_CHANGE_SYNTH_INSTRUMENT, sizeof(SynthProgramChange)},
        {EVENT_REQUEST_CHANGE_SYNTH_INSTRUMENT_QUERY, sizeof(int)},
        {EVENT_REQUEST_DRAW_QUAD, sizeof(Quad)},
        {EVENT_REQUEST_IGNORE_NOTEOFF, sizeof(int)},
        {EVENT_REQUEST_MIDI_MESSAGE_PLAY, sizeof(MidiMessage)},
        {EVENT_REQUEST_MIDI_CHANNEL_STOP, sizeof(int)},
        {EVENT_REQUEST_QUERY, sizeof(QueryRequest)},
        {EVENT_REQUEST_QUIT, sizeof(int)},
        {EVENT_REQUEST_SEQUENCER_START, sizeof(SequencerRequest)},
        {EVENT_REQUEST_SEQUENCER_STOP, 0},
        {EVENT_SEQUENCER_STARTED, sizeof(SequencerRequest)},
        {EVENT_SEQUENCER_STOPPED, 0},
        {EVENT_SEQUENCER_PROGRESS, sizeof(float)},
        {EVENT_SYNTH_INSTRUMENT_CHANGED, sizeof(SynthProgramChange)},
    };

    size_t nDefaultEventTypes = sizeof(defaultEventTypes) / sizeof(defaultEventTypes[0]);
    for (size_t i = 0; i < nDefaultEventTypes; i++) {
        Events_defineNewGlobalEventType(defaultEventTypes[i].name, defaultEventTypes[i].dataSize);
    }
}


void Events_teardown(void) {
    Log_assert(instance, "events instance not created");

    for (const char* eventName = StringMap_iterateInit(instance->eventMap); eventName; eventName = StringMap_iterateNext(instance->eventMap, eventName)) {
        Event* event = StringMap_getItem(instance->eventMap, eventName);
        size_t nSubscribers = HashSet_countItems(event->subscribers);
        if (nSubscribers == 0) {
            HashSet_free(&event->subscribers);
            sfree((void**)&event);
        } else {
            Log_warning("Event '%s' still has %zu active subscribers that were not unsubscribed", eventName, nSubscribers);

        }
    }

    StringMap_free(&instance->eventMap);
    sfree((void**)&instance);
}


void Events_defineNewLocalEventType(const char* eventName, size_t dataSize, void* receiver) {
    Log_assert(instance, "events instance not created");
    Log_assert(receiver, "event receiver is null");
    Events_defineNewEventTypeInternal(eventName, dataSize, receiver);
}


void Event_subscribe(const char* eventName, void* object, void (*callback)(void* self, void* sender, void* data), size_t dataSize) {
    Log_assert(instance, "events instance not created");

    Log_assert(StringMap_containsItem(instance->eventMap, eventName), "Cannot subscribe to non-existing event: '%s'", eventName);
    Event* event = StringMap_getItem(instance->eventMap, eventName);

    Log_assert(dataSize == event->dataSize, "expected data size %zu when subcribing to event '%s', got %zu", event->dataSize, eventName, dataSize);
    Log_assert(object, "null object when subscribing to event '%s'", eventName);
    Log_assert(callback, "null callback when subscribing to event '%s'", eventName);

    if (event->receiver) {
        Log_assert(object == event->receiver, "Invalid receiver %p for local event '%s', expected %p", object, eventName, event->receiver);
    }

    EventSubscriber subscriber = {object, callback};
    bool result = HashSet_addItem(event->subscribers, &subscriber);
    Log_assert(result, "Cannot subscribe to already-subscribed event '%s'", eventName);
}


void Event_unsubscribe(const char* eventName, void* object, void (*callback)(void* self, void* sender, void* data), size_t dataSize) {
    Log_assert(instance, "events instance not created");

    Log_assert(StringMap_containsItem(instance->eventMap, eventName), "Cannot unsubscribe from non-existing event: '%s'", eventName);
    Event* event = StringMap_getItem(instance->eventMap, eventName);

    Log_assert(dataSize == event->dataSize, "expected data size %zu when unsubscribing from event '%s', got %zu", event->dataSize, eventName, dataSize);
    Log_assert(object, "null object when unsubscribing from event '%s'", eventName);
    Log_assert(callback, "null callback when unsubscribing from event '%s'", eventName);

    if (event->receiver) {
        Log_assert(object == event->receiver, "Invalid receiver %p for local event '%s', expected %p", object, eventName, event->receiver);
    }

    EventSubscriber subscriber = {object, callback};
    bool result = HashSet_removeItem(event->subscribers, &subscriber);
    Log_assert(result, "Cannot unsubscribe from non-subscribed event '%s'", eventName);
}


void Event_post(void* sender, const char* eventName, void* data, size_t dataSize) {
    Log_assert(instance, "events instance not created");

    Log_assert(StringMap_containsItem(instance->eventMap, eventName), "Cannot post non-existing event type: '%s'", eventName);
    Event* event = StringMap_getItem(instance->eventMap, eventName);

    Log_assert(dataSize == event->dataSize, "expected data size %zu when posting event '%s', got %zu", event->dataSize, eventName, dataSize);

    for (EventSubscriber* subscriber = HashSet_iterateInit(event->subscribers); subscriber; subscriber = HashSet_iterateNext(event->subscribers, subscriber)) {
        subscriber->callback(subscriber->object, sender, data);
    }
}


static void Events_defineNewGlobalEventType(const char* eventName, size_t dataSize) {
    Log_assert(instance, "events instance not created");
    Events_defineNewEventTypeInternal(eventName, dataSize, NULL);
}


static void Events_defineNewEventTypeInternal(const char* eventName, size_t dataSize, void* receiver) {
    Log_assert(instance, "events instance not created");
    Log_assert(!StringMap_containsItem(instance->eventMap, eventName), "Event type '%s' already exists", eventName);

    Event* event = ecalloc(1, sizeof(Event));
    event->dataSize = dataSize;
    event->receiver = receiver;
    event->subscribers = HashSet_new(sizeof(EventSubscriber));

    StringMap_addItem(instance->eventMap, eventName, event);
}
