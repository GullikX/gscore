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

#include "common/structs/color.h"
#include "common/structs/quad.h"
#include "common/structs/vector2i.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct Grid Grid;
typedef size_t QuadHandle;

Grid* Grid_new(Vector2i size, Vector2 cellSpacing);
void Grid_free(Grid** pself);
QuadHandle Grid_addQuad(Grid* self, Vector2i position, Vector2i size, Color color);
bool Grid_updateQuadPosition(Grid* self, QuadHandle quadHandle, Vector2i position);
bool Grid_updateQuadSize(Grid* self, QuadHandle quadHandle, Vector2i size);
bool Grid_updateQuadColor(Grid* self, QuadHandle quadHandle, Color color);
bool Grid_animateQuadColor(Grid* self, QuadHandle quadHandle, Color colorAnimationTarget, float colorAnimationLerpWeight);
void Grid_finalizeColorAnimation(Grid* self, QuadHandle quadHandle);
void Grid_finalizeAllColorAnimations(Grid* self);
void Grid_hide(Grid* self);
void Grid_unhide(Grid* self);
void Grid_hideQuad(Grid* self, QuadHandle quadHandle);
void Grid_unhideQuad(Grid* self, QuadHandle quadHandle);
void Grid_zoomTo(Grid* self, Vector2i zoomRectPosition, Vector2i zoomRectSize);
