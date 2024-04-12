#pragma once

#include "common/structs/color.h"

#pragma pack(push, 1)
typedef struct {
    int iTrack;
    int iTimeSlot;
    Color color;
} BlockInstance;
#pragma pack(pop)
