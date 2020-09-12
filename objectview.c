/* Copyright (C) 2020 Martin Gulliksson <martin@gullik.cc>
 *
 * This file is part of gscore.
 *
 * gscore is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, version 3.
 *
 * gscore is distributed in the hope that it will be useful,
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

    self->score = score;

    int nColumns = SCORE_LENGTH_MAX;
    const char* trackColors[2] = {COLOR_GRIDLINES, COLOR_BACKGROUND};
    int iTrackColor = 0;

    for (int i = 0; i < N_TRACKS_MAX; i++) {
        self->gridlinesHorizontal[i].iRow = i;
        self->gridlinesHorizontal[i].iColumn = 0;
        self->gridlinesHorizontal[i].nRows = 1;
        self->gridlinesHorizontal[i].nColumns = nColumns;
        bool success = hexColorToRgb(trackColors[iTrackColor], &self->gridlinesHorizontal[i].color);
        if (!success) die("Invalid gridline color");
        self->gridlinesHorizontal[i].indicatorValue = -1.0f;
        iTrackColor = !iTrackColor;
    }

    self->cursor.iRow = 0;
    self->cursor.iColumn = 0;
    self->cursor.nRows = 1;
    self->cursor.nColumns = 1;
    bool success = hexColorToRgb(COLOR_CURSOR, &self->cursor.color);
    if (!success) die("Invalid cursor color");
    self->cursor.indicatorValue = -1.0f;

    self->playStartTime = -1;
    success = hexColorToRgb(COLOR_PLAYBACK_CURSOR, &self->playbackCursorColor);
    if (!success) die("Invalid playback cursor color");

    self->ctrlPressed = false;

    return self;
}


ObjectView* ObjectView_free(ObjectView* self) {
    free(self);
    return NULL;
}


void ObjectView_addBlock(ObjectView* self) {
    if (ObjectView_isPlaying(self)) return; /* TODO: allow this */
    int iTrack = self->cursor.iRow;
    int iBlock = self->cursor.iColumn;
    if (iTrack < 0 || iTrack >= self->score->nTracks || iBlock < 0 || iBlock >= self->score->scoreLength) return;

    Track* track = Application_getInstance()->scoreCurrent->tracks[iTrack];
    Block** block = Application_getInstance()->blockCurrent;

    Track_setBlock(track, iBlock, block);
    Track_setBlockVelocity(track, iBlock, DEFAULT_VELOCITY);
}


void ObjectView_removeBlock(ObjectView* self) {
    if (ObjectView_isPlaying(self)) return; /* TODO: allow this */
    int iTrack = self->cursor.iRow;
    int iBlock = self->cursor.iColumn;
    if (iTrack < 0 || iTrack >= self->score->nTracks || iBlock < 0 || iBlock >= self->score->scoreLength) return;

    Track* track = Application_getInstance()->scoreCurrent->tracks[iTrack];
    Track_setBlock(track, iBlock, NULL);
}


void ObjectView_setCtrlPressed(ObjectView* self, bool ctrlPressed) {
    self->ctrlPressed = ctrlPressed;
}


void ObjectView_playScore(ObjectView* self, float startPosition, bool repeat) {
    self->iPlayStartBlock = startPosition * self->score->scoreLength;
    self->playRepeat = repeat;
    float blockTime = (float)(BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE) / (float)self->score->tempo;
    float stopTime = -1.0f;

    for (int iTrack = 0; iTrack < self->score->nTracks; iTrack++) {
        for (int iBlock = 0; iBlock < self->score->scoreLength; iBlock++) {
            if (iBlock < self->iPlayStartBlock) continue;
            if (!self->score->tracks[iTrack]->blocks[iBlock]) continue;
            Block* block = *self->score->tracks[iTrack]->blocks[iBlock];
            for (MidiMessage* midiMessage = block->midiMessageRoot; midiMessage; midiMessage = midiMessage->next) {
                if (midiMessage->time < 0) continue;
                int channel = iTrack + 1;
                float velocity = midiMessage->velocity * self->score->tracks[iTrack]->velocity * self->score->tracks[iTrack]->blockVelocities[iBlock];
                float time = midiMessage->time * blockTime + blockTime * (iBlock - self->iPlayStartBlock);
                if (stopTime < time) stopTime = time;

                switch (midiMessage->type) {
                    case FLUID_SEQ_NOTEON:
                        Synth_sendNoteOn(Application_getInstance()->synth, channel, midiMessage->pitch, velocity, time);
                        break;
                    case FLUID_SEQ_NOTEOFF:
                        if (!self->score->tracks[iTrack]->ignoreNoteOff) {
                            Synth_sendNoteOff(Application_getInstance()->synth, channel, midiMessage->pitch, time);
                        }
                        break;
                }
            }
        }
    }
    Synth_scheduleCallback(Application_getInstance()->synth, stopTime);
    self->playStartTime = Synth_getTime(Application_getInstance()->synth);
}


void ObjectView_sequencerCallback(ObjectView* self) {
    if (self->playRepeat) {
        ObjectView_playScore(self, 0.0f, true);
    }
    else {
        ObjectView_stopPlaying(self);
    }
}


void ObjectView_stopPlaying(ObjectView* self) {
    Synth_noteOffAll(Application_getInstance()->synth);
    self->playStartTime = -1;
}


bool ObjectView_isPlaying(ObjectView* self) {
    return self->playStartTime > 0;
}


void ObjectView_setProgram(ObjectView* self, const char* const programName) {
    int iTrack = self->cursor.iRow;
    int iBlock = self->cursor.iColumn;
    if (iTrack < 0 || iTrack >= self->score->nTracks || iBlock < 0 || iBlock >= self->score->scoreLength) return;
    Track* track = self->score->tracks[iTrack];
    Track_setProgram(track, programName);
    Synth_setProgramByName(Application_getInstance()->synth, iTrack + 1, self->score->tracks[iTrack]->programName);
}


void ObjectView_toggleIgnoreNoteOff(ObjectView* self) {
    int iTrack = self->cursor.iRow;
    int iBlock = self->cursor.iColumn;
    if (iTrack < 0 || iTrack >= self->score->nTracks || iBlock < 0 || iBlock >= self->score->scoreLength) return;
    Track* track = self->score->tracks[iTrack];
    Track_toggleIgnoreNoteOff(track);
}


void ObjectView_draw(ObjectView* self) {
    self->viewHeight = self->score->nTracks * MAX_TRACK_HEIGHT;

    for (int i = 0; i < self->score->nTracks; i++) {
        ObjectView_drawItem(self, &(self->gridlinesHorizontal[i]), 0);
    }

    if (self->cursor.iRow < self->score->nTracks) {
        ObjectView_drawItem(self, &(self->cursor), CURSOR_SIZE_OFFSET);
    }

    Track** tracks = Application_getInstance()->scoreCurrent->tracks;
    for (int iTrack = 0; iTrack < self->score->nTracks; iTrack++) {
        for (int iBlock = 0; iBlock < self->score->scoreLength; iBlock++) {
            if (!tracks[iTrack]->blocks[iBlock]) continue;
            Block* block = *tracks[iTrack]->blocks[iBlock];
            Vector4 color = block->color;

            bool highlight = iTrack == self->cursor.iRow && iBlock == self->cursor.iColumn;
            if (highlight) {
                color.x = HIGHLIGHT_STRENGTH * 1.0f + (1.0f - HIGHLIGHT_STRENGTH) * color.x;
                color.y = HIGHLIGHT_STRENGTH * 1.0f + (1.0f - HIGHLIGHT_STRENGTH) * color.y;
                color.z = HIGHLIGHT_STRENGTH * 1.0f + (1.0f - HIGHLIGHT_STRENGTH) * color.z;
            }

            GridItem item;
            item.iRow = iTrack;
            item.iColumn = iBlock;
            item.nRows = 1;
            item.nColumns = 1;
            item.color = color;
            item.indicatorValue = tracks[iTrack]->blockVelocities[iBlock];
            ObjectView_drawItem(self, &(item), BLOCK_SIZE_OFFSET);
        }
    }

    ObjectView_drawPlaybackCursor(self);
}


void ObjectView_drawItem(ObjectView* self, GridItem* item, float offset) {
    float columnWidth = 2.0f/(float)self->score->scoreLength;
    float rowHeight = self->viewHeight / self->score->nTracks;

    float x1 = -1.0f + item->iColumn * columnWidth - offset;
    float x2 = -1.0f + item->iColumn * columnWidth + item->nColumns * columnWidth + offset;
    float y1 = -(-1.0f + item->iRow * rowHeight) + offset;
    float y2 = -(-1.0f + item->iRow * rowHeight + item->nRows * rowHeight) - offset;

    Renderer_drawQuad(Application_getInstance()->renderer, x1, x2, y1, y2, item->color);

    if (self->ctrlPressed && item->indicatorValue > 0.0f) {
        Vector4 indicatorColor = {1.0f - item->color.x, 1.0f - item->color.y, 1.0f - item->color.z, 1.0f};
        x1 = -1.0f + item->iColumn * columnWidth - offset;
        x2 = -1.0f + item->iColumn * columnWidth + columnWidth * VELOCITY_INDICATOR_WIDTH_OBJECT_MODE + offset;
        y1 = -(-1.0f + (item->iRow + 1.0f - item->indicatorValue) * rowHeight) + offset;
        y2 = -(-1.0f + item->iRow * rowHeight + item->nRows * rowHeight) - offset;
        Renderer_drawQuad(Application_getInstance()->renderer, x1, x2, y1, y2, indicatorColor);
    }
}


void ObjectView_drawPlaybackCursor(ObjectView* self) {
    if (!ObjectView_isPlaying(self)) return;

    float time = Synth_getTime(Application_getInstance()->synth) - self->playStartTime;
    float totalTime = 1000.0f * (float)(self->score->scoreLength * BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE) / (float)self->score->tempo;
    float progress = time / totalTime + (float)self->iPlayStartBlock / (float)self->score->scoreLength;
    float cursorX = -1.0f + 2.0f * progress;

    float x1 = cursorX - PLAYER_CURSOR_WIDTH / 2.0f;
    float x2 = cursorX + PLAYER_CURSOR_WIDTH / 2.0f;
    float y1 = -1.0f;
    float y2 = 1.0f;

    Renderer_drawQuad(Application_getInstance()->renderer, x1, x2, y1, y2, self->playbackCursorColor);
}


bool ObjectView_updateCursorPosition(ObjectView* self, float x, float y) {
    int iColumnOld = self->cursor.iColumn;
    int iRowOld = self->cursor.iRow;

    self->cursor.iColumn = ObjectView_xCoordToColumnIndex(self, x);
    self->cursor.iRow = ObjectView_yCoordToRowIndex(self, y);

    return self->cursor.iColumn == iColumnOld && self->cursor.iRow == iRowOld ? false : true;
}


int ObjectView_xCoordToColumnIndex(ObjectView* self, float x) {
    int nColumns = self->score->scoreLength;
    return (nColumns * x) / Application_getInstance()->renderer->viewportWidth;
}


int ObjectView_yCoordToRowIndex(ObjectView* self, float y) {
    int nRows = self->score->nTracks;
    return (nRows * y) / (Application_getInstance()->renderer->viewportHeight * self->viewHeight / 2.0f);
}
