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

ObjectView* ObjectView_new(Score* score) {
    ObjectView* self = ecalloc(1, sizeof(*self));

    int nColumns = SCORE_LENGTH;
    Vector4 trackColors[2] = {COLOR_BACKGROUND, COLOR_GRIDLINES};
    int iTrackColor = 0;

    for (int i = 0; i < N_TRACKS; i++) {
        self->gridlinesHorizontal[i].iRow = i;
        self->gridlinesHorizontal[i].iColumn = 0;
        self->gridlinesHorizontal[i].nRows = 1;
        self->gridlinesHorizontal[i].nColumns = nColumns;
        self->gridlinesHorizontal[i].color = trackColors[iTrackColor];
        iTrackColor = !iTrackColor;
    }

    self->viewHeight = N_TRACKS * MAX_TRACK_HEIGHT;

    self->cursor.iRow = 0;
    self->cursor.iColumn = 0;
    self->cursor.nRows = 1;
    self->cursor.nColumns = 1;
    self->cursor.color = COLOR_CURSOR;

    self->player = ScorePlayer_new(score);

    return self;
}


ObjectView* ObjectView_free(ObjectView* self) {
    free(self);
    return NULL;
}


void ObjectView_update(ObjectView* self) {
    ScorePlayer_update(self->player);
}


void ObjectView_addBlock(ObjectView* self) {
    if (ScorePlayer_playing(self->player)) return; /* TODO: allow this */
    int iTrack = self->cursor.iRow;
    int iBlock = self->cursor.iColumn;
    if (iTrack < 0 || iTrack >= N_TRACKS || iBlock < 0 || iBlock >= SCORE_LENGTH) return;

    Application* application = Application_getInstance();
    application->scoreCurrent->tracks[iTrack].blocks[iBlock] = application->blockCurrent;
    application->scoreCurrent->tracks[iTrack].blockVelocities[iBlock] = DEFAULT_VELOCITY;
}


void ObjectView_removeBlock(ObjectView* self) {
    if (ScorePlayer_playing(self->player)) return; /* TODO: allow this */
    int iTrack = self->cursor.iRow;
    int iBlock = self->cursor.iColumn;
    if (iTrack < 0 || iTrack >= N_TRACKS || iBlock < 0 || iBlock >= SCORE_LENGTH) return;

    Application_getInstance()->scoreCurrent->tracks[iTrack].blocks[iBlock] = NULL;
}


void ObjectView_draw(ObjectView* self) {
    for (int i = 0; i < N_TRACKS; i++) {
        ObjectView_drawItem(self, &(self->gridlinesHorizontal[i]), 0);
    }

    Track* tracks = Application_getInstance()->scoreCurrent->tracks;
    for (int iTrack = 0; iTrack < N_TRACKS; iTrack++) {
        for (int iBlock = 0; iBlock < SCORE_LENGTH; iBlock++) {
            Block* block = tracks[iTrack].blocks[iBlock];
            if (!block) continue;
            GridItem item;
            item.iRow = iTrack;
            item.iColumn = iBlock;
            item.nRows = 1;
            item.nColumns = 1;
            item.color = block->color;
            ObjectView_drawItem(self, &(item), BLOCK_SIZE_OFFSET);
        }
    }

    if (self->cursor.iRow < N_TRACKS) {
        ObjectView_drawItem(self, &(self->cursor), CURSOR_SIZE_OFFSET);
    }

    ScorePlayer_drawCursor(self->player);
}


void ObjectView_drawItem(ObjectView* self, GridItem* item, float offset) {
    float columnWidth = 2.0f/SCORE_LENGTH;
    float rowHeight = self->viewHeight / N_TRACKS;

    float x1 = -1.0f + item->iColumn * columnWidth - offset;
    float x2 = -1.0f + item->iColumn * columnWidth + item->nColumns * columnWidth + offset;
    float y1 = -(-1.0f + item->iRow * rowHeight) + offset;
    float y2 = -(-1.0f + item->iRow * rowHeight + item->nRows * rowHeight) - offset;

    Renderer_drawQuad(Application_getInstance()->renderer, x1, x2, y1, y2, item->color);
}


bool ObjectView_updateCursorPosition(ObjectView* self, float x, float y) {
    int iColumnOld = self->cursor.iColumn;
    int iRowOld = self->cursor.iRow;

    self->cursor.iColumn = ObjectView_xCoordToColumnIndex(x);
    self->cursor.iRow = ObjectView_yCoordToRowIndex(self, y);

    return self->cursor.iColumn == iColumnOld && self->cursor.iRow == iRowOld ? false : true;
}


int ObjectView_xCoordToColumnIndex(float x) {
    int nColumns = SCORE_LENGTH;
    return (nColumns * x) / Application_getInstance()->renderer->viewportWidth;
}


int ObjectView_yCoordToRowIndex(ObjectView* self, float y) {
    int nRows = N_TRACKS;
    return (nRows * y) / (Application_getInstance()->renderer->viewportHeight * self->viewHeight / 2.0f);
}
