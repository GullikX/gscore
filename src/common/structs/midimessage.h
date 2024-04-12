#pragma once

#pragma pack(push, 1)
typedef struct {
    int type;
    int channel;
    int pitch;
    int velocity;
    float timestampSeconds;
} MidiMessage;
#pragma pack(pop)
