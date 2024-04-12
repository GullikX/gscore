#pragma once

#include "common/constants/fluidmidi.h"

enum {
    SYNTH_PROGRAM_CHANGE_NAME_BUFFER_SIZE = MIDI_SYNTH_PROGRAM_NAME_LENGTH_MAX + 1, // extra space for null terminator
};

#pragma pack(push, 1)
typedef struct {
    char name[SYNTH_PROGRAM_CHANGE_NAME_BUFFER_SIZE];
    int iChannel;
} SynthProgramChange;
#pragma pack(pop)
