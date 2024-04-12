#pragma once

#include "midimessage.h"

#include <stddef.h>

#pragma pack(push, 1)
typedef struct {
    float timestampStart;
    float timestampEnd;
    size_t nMidiMessages;
    MidiMessage* midiMessages;
} SequencerRequest;
#pragma pack(pop)
