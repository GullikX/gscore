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

#include "grid.h"

#include "common/util/alloc.h"
#include "common/util/colors.h"
#include "common/util/hash.h"
#include "common/util/hashmap.h"
#include "common/util/log.h"
#include "common/structs/color.h"
#include "common/structs/quad.h"
#include "common/structs/vector2i.h"
#include "events/events.h"

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct GridQuad GridQuad;
struct GridQuad {
    Vector2i position;
    Vector2i size;
    Color color;
    Color colorAnimationTarget;
    float colorAnimationLerpWeight;
    bool isVisible;
};
#pragma pack(pop)

struct Grid {
    Vector2i size;
    QuadHandle quadHandlePrevious;
    HashMap* gridQuads;
    bool isVisible;
    Vector2 cellSpacing;
    Vector2i zoomRectPosition;
    Vector2i zoomRectSize;
};


static void Grid_onProcessFrame(Grid* self, void* sender, float* deltaTime);
static Quad Grid_createRenderQuad(Grid* self, GridQuad* gridQuad);
static QuadHandle Grid_generateNewQuadHandle(Grid* self);
static void Grid_validateQuadHandle(Grid* self, QuadHandle quadHandle);


Grid* Grid_new(Vector2i size, Vector2 cellSpacing) {
    Grid* self = ecalloc(1, sizeof(*self));

    Log_assert(size.x > 0 && size.y > 0, "Size must be positive, got (%d, %d)", size.x, size.y);
    self->size = size;
    self->cellSpacing = cellSpacing;
    self->quadHandlePrevious = (QuadHandle)self; // 'random' seed
    self->zoomRectPosition = (Vector2i){0, 0};
    self->zoomRectSize = self->size;

    self->gridQuads = HashMap_new(sizeof(QuadHandle));

    Grid_unhide(self);

    return self;
}


void Grid_free(Grid** pself) {
    Grid* self = *pself;
    Grid_hide(self);

    for (QuadHandle* quadHandle = HashMap_iterateInit(self->gridQuads); quadHandle; quadHandle = HashMap_iterateNext(self->gridQuads, quadHandle)) {
        GridQuad* gridQuad = HashMap_getItem(self->gridQuads, quadHandle);
        sfree((void**)&gridQuad);
    }
    HashMap_free(&self->gridQuads);

    sfree((void**)pself);
}


QuadHandle Grid_addQuad(Grid* self, Vector2i position, Vector2i size, Color color) {
    QuadHandle quadHandle = Grid_generateNewQuadHandle(self);

    bool isVisible = true;
    GridQuad gridQuad = {position, size, color, color, 0.0, isVisible};
    GridQuad* gridQuadHeap = ecalloc(1, sizeof(GridQuad));
    *gridQuadHeap = gridQuad;
    HashMap_addItem(self->gridQuads, &quadHandle, gridQuadHeap);

    return quadHandle;
}


bool Grid_updateQuadPosition(Grid* self, QuadHandle quadHandle, Vector2i position) {
    Grid_validateQuadHandle(self, quadHandle);

    GridQuad* gridQuad = HashMap_getItem(self->gridQuads, &quadHandle);

    if (memcmp(&gridQuad->position, &position, sizeof(Vector2i))) {
        gridQuad->position = position;
        return true;
    }

    return false;
}


bool Grid_updateQuadSize(Grid* self, QuadHandle quadHandle, Vector2i size) {
    Grid_validateQuadHandle(self, quadHandle);

    GridQuad* gridQuad = HashMap_getItem(self->gridQuads, &quadHandle);

    if (memcmp(&gridQuad->size, &size, sizeof(Vector2i))) {
        gridQuad->size = size;
        return true;
    }

    return false;
}


bool Grid_updateQuadColor(Grid* self, QuadHandle quadHandle, Color color) {
    Grid_validateQuadHandle(self, quadHandle);

    GridQuad* gridQuad = HashMap_getItem(self->gridQuads, &quadHandle);

    if (memcmp(&gridQuad->color, &color, sizeof(Color))) {
        gridQuad->color = color;
        gridQuad->colorAnimationTarget = color;
        gridQuad->colorAnimationLerpWeight = 0.0;
        return true;
    }

    return false;
}


bool Grid_animateQuadColor(Grid* self, QuadHandle quadHandle, Color colorAnimationTarget, float colorAnimationLerpWeight) {
    Grid_validateQuadHandle(self, quadHandle);

    GridQuad* gridQuad = HashMap_getItem(self->gridQuads, &quadHandle);

    if (memcmp(&gridQuad->colorAnimationTarget, &colorAnimationTarget, sizeof(Color)) || gridQuad->colorAnimationLerpWeight != colorAnimationLerpWeight) {
        gridQuad->colorAnimationTarget = colorAnimationTarget;
        gridQuad->colorAnimationLerpWeight = colorAnimationLerpWeight;
        return true;
    }

    return false;
}


void Grid_finalizeColorAnimation(Grid* self, QuadHandle quadHandle) {
    Grid_validateQuadHandle(self, quadHandle);

    GridQuad* gridQuad = HashMap_getItem(self->gridQuads, &quadHandle);
    gridQuad->color = gridQuad->colorAnimationTarget;
    gridQuad->colorAnimationLerpWeight = 0.0;
}


void Grid_finalizeAllColorAnimations(Grid* self) {
    for (QuadHandle* quadHandle = HashMap_iterateInit(self->gridQuads); quadHandle; quadHandle = HashMap_iterateNext(self->gridQuads, quadHandle)) {
        Grid_finalizeColorAnimation(self, *quadHandle);
    }
}


void Grid_hide(Grid* self) {
    if (self->isVisible) {
        Event_unsubscribe(EVENT_PROCESS_FRAME, self, EVENT_CALLBACK(Grid_onProcessFrame), sizeof(float));
        self->isVisible = false;
    }
}


void Grid_unhide(Grid* self) {
    if (!self->isVisible) {
        Event_subscribe(EVENT_PROCESS_FRAME, self, EVENT_CALLBACK(Grid_onProcessFrame), sizeof(float));
        self->isVisible = true;
    }
}


void Grid_hideQuad(Grid* self, QuadHandle quadHandle) {
    Grid_validateQuadHandle(self, quadHandle);
    GridQuad* gridQuad = HashMap_getItem(self->gridQuads, &quadHandle);
    gridQuad->isVisible = false;
}


void Grid_unhideQuad(Grid* self, QuadHandle quadHandle) {
    Grid_validateQuadHandle(self, quadHandle);
    GridQuad* gridQuad = HashMap_getItem(self->gridQuads, &quadHandle);
    gridQuad->isVisible = true;
}


void Grid_zoomTo(Grid* self, Vector2i zoomRectPosition, Vector2i zoomRectSize) {
    Log_assert(zoomRectPosition.x >= 0 && zoomRectPosition.x < self->size.x, "Invalid zoom x position, %d", zoomRectPosition.x);
    Log_assert(zoomRectPosition.y >= 0 && zoomRectPosition.y < self->size.y, "Invalid zoom y position, %d", zoomRectPosition.y);
    self->zoomRectPosition = zoomRectPosition;

    Log_assert(zoomRectSize.x > 0 && zoomRectPosition.x + zoomRectSize.x <= self->size.x, "Invalid zoom x size, %d", zoomRectSize.x);
    Log_assert(zoomRectSize.y > 0 && zoomRectPosition.y + zoomRectSize.y <= self->size.y, "Invalid zoom y size, %d", zoomRectSize.y);
    self->zoomRectSize = zoomRectSize;
}


static void Grid_onProcessFrame(Grid* self, void* sender, float* deltaTime) {
    (void)sender; (void)deltaTime;
    for (QuadHandle* quadHandle = HashMap_iterateInit(self->gridQuads); quadHandle; quadHandle = HashMap_iterateNext(self->gridQuads, quadHandle)) {
        GridQuad* gridQuad = HashMap_getItem(self->gridQuads, quadHandle);
        if (gridQuad->isVisible) {
            if (gridQuad->colorAnimationLerpWeight > 0) {
                Color color = gridQuad->color;
                Color colorTarget = gridQuad->colorAnimationTarget;
                float lerpWeight = gridQuad->colorAnimationLerpWeight;
                gridQuad->color = lerpColorDeltaTimeAdjusted(color, colorTarget, lerpWeight, *deltaTime);
            }
            Quad renderQuad = Grid_createRenderQuad(self, gridQuad);
            Event_post(self, EVENT_REQUEST_DRAW_QUAD, &renderQuad, sizeof(renderQuad));
        }
    }
}


static Quad Grid_createRenderQuad(Grid* self, GridQuad* gridQuad) {
    Vector2 position = {
        (float)(gridQuad->position.x - self->zoomRectPosition.x) / self->zoomRectSize.x + self->cellSpacing.x,
        (float)(gridQuad->position.y - self->zoomRectPosition.y) / self->zoomRectSize.y + self->cellSpacing.y,
    };
    Vector2 size = {
        (float)gridQuad->size.x / self->zoomRectSize.x - 2.0f * self->cellSpacing.x,
        (float)gridQuad->size.y / self->zoomRectSize.y - 2.0f * self->cellSpacing.y,
    };
    return (Quad){position, size, gridQuad->color};
}


static QuadHandle Grid_generateNewQuadHandle(Grid* self) {
    QuadHandle quadHandle = hashDjb2((char*)&self->quadHandlePrevious, sizeof(self->quadHandlePrevious));
    Log_assert(!HashMap_containsItem(self->gridQuads, &quadHandle), "Hash collision in grid");
    Log_assert(quadHandle != 0, "Generated grid quad handle is zero");
    self->quadHandlePrevious = quadHandle;
    return quadHandle;
}


static void Grid_validateQuadHandle(Grid* self, QuadHandle quadHandle) {
    Log_assert(HashMap_containsItem(self->gridQuads, &quadHandle), "Invalid quad handle");
}
