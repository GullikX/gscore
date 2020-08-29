/* Copyright (C) 2020 Martin Gulliksson <martin@gullik.cc>
 *
 * This file is part of GScore.
 *
 * GScore is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, version 3.
 *
 * GScore is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>
 *
 */

ObjectView* ObjectView_getInstance(void) {
    static ObjectView* self = NULL;
    if (self) return self;

    self = ecalloc(1, sizeof(*self));

    int nColumns = SCORE_LENGTH;
    Vector4 trackColors[] = {COLOR_BACKGROUND, COLOR_GRIDLINES};
    int iTrackColor = 0;

    for (int i = 0; i < N_TRACKS; i++) {
        self->gridlinesHorizontal[i].iRow = i;
        self->gridlinesHorizontal[i].iColumn = 0;
        self->gridlinesHorizontal[i].nRows = 1;
        self->gridlinesHorizontal[i].nColumns = nColumns;
        self->gridlinesHorizontal[i].color = trackColors[iTrackColor];
        iTrackColor = !iTrackColor;
    }

    self->cursor.iRow = 0;
    self->cursor.iColumn = 0;
    self->cursor.nRows = 1;
    self->cursor.nColumns = 1;
    self->cursor.color = COLOR_CURSOR;

    return self;
}


void ObjectView_draw(void) {
    ObjectView* self = ObjectView_getInstance();

    for (int i = 0; i < OCTAVES; i++) {
        ObjectView_drawItem(&(self->gridlinesHorizontal[i]), 0);
    }

    ObjectView_drawItem(&(self->cursor), CURSOR_SIZE_OFFSET);
}


void ObjectView_drawItem(GridItem* item, float offset) {
    float columnWidth = 2.0f/SCORE_LENGTH;
    float rowHeight = 2.0f/N_TRACKS;

    float x1 = -1.0f + item->iColumn * columnWidth - offset;
    float x2 = -1.0f + item->iColumn * columnWidth + item->nColumns * columnWidth + offset;
    float y1 = -(-1.0f + item->iRow * rowHeight) + offset;
    float y2 = -(-1.0f + item->iRow * rowHeight + item->nRows * rowHeight) - offset;

    Renderer_drawQuad(x1, x2, y1, y2, item->color);
}


bool ObjectView_updateCursorPosition(float x, float y) {
    ObjectView* self = ObjectView_getInstance();

    int iColumnOld = self->cursor.iColumn;
    int iRowOld = self->cursor.iRow;

    self->cursor.iColumn = ObjectView_xCoordToColumnIndex(x);
    self->cursor.iRow = ObjectView_yCoordToRowIndex(y);

    return self->cursor.iColumn == iColumnOld && self->cursor.iRow == iRowOld ? false : true;
}


int ObjectView_xCoordToColumnIndex(int x) {
    int nColumns = SCORE_LENGTH;
    return (nColumns * x) / Renderer_getInstance()->viewportWidth;
}


int ObjectView_yCoordToRowIndex(int y) {
    int nRows = N_TRACKS;
    return (nRows * y) / Renderer_getInstance()->viewportHeight;
}
