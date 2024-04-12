#pragma once

#include "common/structs/color.h"
#include "common/structs/vector2.h"

#pragma pack(push, 1)
typedef struct {
    Vector2 position;
    Vector2 size;
    Color color;
} Quad;
#pragma pack(pop)
