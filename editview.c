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

static EditView* EditView_new(Score* score) {
    EditView* self = ecalloc(1, sizeof(*self));

    self->score = score;

    int nRows = OCTAVES*NOTES_IN_OCTAVE;
    int nColumns = BLOCK_MEASURES * score->nBeatsPerMeasure * MEASURE_RESOLUTION;

    for (int i = 0; i < BLOCK_MEASURES; i++) {
        self->gridlinesVertical[i].iRow = 0;
        self->gridlinesVertical[i].iColumn = i * score->nBeatsPerMeasure * MEASURE_RESOLUTION;
        self->gridlinesVertical[i].nRows = nRows;
        self->gridlinesVertical[i].nColumns = 1;
        bool success = hexColorToRgb(COLOR_GRIDLINES, &self->gridlinesVertical[i].color);
        if (!success) die("Invalid gridline color '%s'", COLOR_GRIDLINES);
        self->gridlinesVertical[i].indicatorValue = -1.0f;
    }

    for (int i = 0; i < OCTAVES*NOTES_IN_OCTAVE; i++) {
        self->gridlinesHorizontal[i].iRow = i;
        self->gridlinesHorizontal[i].iColumn = 0;
        self->gridlinesHorizontal[i].nRows = 1;
        self->gridlinesHorizontal[i].nColumns = nColumns;
        bool success = hexColorToRgb(COLOR_GRIDLINES, &self->gridlinesHorizontal[i].color);
        if (!success) die("Invalid gridline color '%s'", COLOR_GRIDLINES);
        self->gridlinesHorizontal[i].indicatorValue = -1.0f;
    }

    self->cursor.iRow = 0;
    self->cursor.iColumn = 0;
    self->cursor.nRows = 1;
    self->cursor.nColumns = 1;
    bool success = hexColorToRgb(COLOR_CURSOR, &self->cursor.color);
    if (!success) die("Invalid cursor color '%s'", COLOR_CURSOR);
    self->cursor.indicatorValue = -1.0f;

    self->playStartTime = -1;
    self->tempo = score->tempo;
    success = hexColorToRgb(COLOR_PLAYBACK_CURSOR, &self->playbackCursorColor);
    if (!success) die("Invalid playback cursor color '%s'", COLOR_PLAYBACK_CURSOR);

    self->ctrlPressed = false;

    self->ignoreNoteOff = IGNORE_NOTE_OFF_DEFAULT;

    return self;
}


static EditView* EditView_free(EditView* self) {
    free(self);
    return NULL;
}


static void EditView_previewNote(EditView* self) {
    int pitch = EditView_rowIndexToNoteKey(self->cursor.iRow);
    if (!EditView_isPlaying(self)) {
        Synth_noteOffAll(Application_getInstance()->synth);
        Synth_noteOn(Application_getInstance()->synth, pitch);
    }
    printf("Previewing note %s%d\n", NOTE_NAMES[pitch % NOTES_IN_OCTAVE], pitch / NOTES_IN_OCTAVE - 1);
}


static void EditView_addNote(EditView* self) {
    if (EditView_isPlaying(self)) return; /* TODO: allow this */
    int nColumns = BLOCK_MEASURES * self->score->nBeatsPerMeasure * MEASURE_RESOLUTION;
    int pitch = EditView_rowIndexToNoteKey(self->cursor.iRow);
    float velocity = DEFAULT_VELOCITY;

    Synth_noteOn(Application_getInstance()->synth, pitch);

    float timeStart = (float)self->cursor.iColumn / (float)nColumns;
    EditView_addMidiMessage(FLUID_SEQ_NOTEON, timeStart, pitch, velocity);

    float timeEnd = (float)(self->cursor.iColumn + 1) / (float)nColumns;
    self->midiMessageHeld = EditView_addMidiMessage(FLUID_SEQ_NOTEOFF, timeEnd, pitch, 0);

    printf("Adding note %s%d\n", NOTE_NAMES[pitch % NOTES_IN_OCTAVE], pitch / NOTES_IN_OCTAVE - 1);
}


static void EditView_dragNote(EditView* self) {
    if (EditView_isPlaying(self)) return; /* TODO: allow this */
    if (!self->midiMessageHeld) return;

    int nColumns = BLOCK_MEASURES * self->score->nBeatsPerMeasure * MEASURE_RESOLUTION;
    float time = (float)(self->cursor.iColumn + 1) / (float)nColumns;
    int pitch = self->midiMessageHeld->pitch;
    float velocity = self->midiMessageHeld->velocity;

    EditView_removeMidiMessage(self->midiMessageHeld);
    self->midiMessageHeld = EditView_addMidiMessage(FLUID_SEQ_NOTEOFF, time, pitch, velocity);
}


static void EditView_releaseNote(EditView* self) {
    self->midiMessageHeld = NULL;
    if (!EditView_isPlaying(self)) {
        Synth_noteOffAll(Application_getInstance()->synth);
    }
}


static void EditView_removeNote(EditView* self) {
    if (EditView_isPlaying(self)) return; /* TODO: allow this */
    int nColumns = BLOCK_MEASURES * self->score->nBeatsPerMeasure * MEASURE_RESOLUTION;
    float time = (float)self->cursor.iColumn / (float)nColumns;
    int pitch = EditView_rowIndexToNoteKey(self->cursor.iRow);

    Block* blockCurrent = *Application_getInstance()->blockCurrent;
    MidiMessage* midiMessage = blockCurrent->midiMessageRoot;

    while (midiMessage) {
        if (midiMessage->type == FLUID_SEQ_NOTEON) {
            MidiMessage* midiMessageOther = midiMessage;
            while (midiMessageOther) {
                if (midiMessageOther->type == FLUID_SEQ_NOTEOFF && midiMessageOther->pitch == midiMessage->pitch) {
                    if (midiMessage->pitch == pitch && midiMessage->time <= time && midiMessageOther->time > time) {
                        EditView_removeMidiMessage(midiMessage);
                        EditView_removeMidiMessage(midiMessageOther);
                    }
                    break;
                }
                midiMessageOther = midiMessageOther->next;
            }
        }
        midiMessage = midiMessage->next;
    }
}


static void EditView_adjustNoteVelocity(EditView* self, float amount) {
    if (EditView_isPlaying(self)) return; /* TODO: allow this */
    int nColumns = BLOCK_MEASURES * self->score->nBeatsPerMeasure * MEASURE_RESOLUTION;
    float time = (float)self->cursor.iColumn / (float)nColumns;
    int pitch = EditView_rowIndexToNoteKey(self->cursor.iRow);

    Block* blockCurrent = *Application_getInstance()->blockCurrent;
    MidiMessage* midiMessage = blockCurrent->midiMessageRoot;

    while (midiMessage) {
        if (midiMessage->type == FLUID_SEQ_NOTEON) {
            MidiMessage* midiMessageOther = midiMessage;
            while (midiMessageOther) {
                if (midiMessageOther->type == FLUID_SEQ_NOTEOFF && midiMessageOther->pitch == midiMessage->pitch) {
                    if (midiMessage->pitch == pitch && midiMessage->time <= time && midiMessageOther->time > time) {
                        midiMessage->velocity += amount;
                        if (midiMessage->velocity > 1.0f) midiMessage->velocity = 1.0f;
                        if (midiMessage->velocity < 0.0f) midiMessage->velocity = 0.0f;
                        printf("Note velocity: %f\n", midiMessage->velocity);
                    }
                    break;
                }
                midiMessageOther = midiMessageOther->next;
            }
        }
        midiMessage = midiMessage->next;
    }
}


static void EditView_setCtrlPressed(EditView* self, bool ctrlPressed) {
    self->ctrlPressed = ctrlPressed;
}


static void EditView_playBlock(EditView* self, float startPosition, bool repeat) {
    self->playStartPosition = startPosition;
    self->playRepeat = repeat;
    float blockTime = (float)(BLOCK_MEASURES * self->score->nBeatsPerMeasure * SECONDS_PER_MINUTE) / (float)self->tempo;
    Block* blockCurrent = *Application_getInstance()->blockCurrent;
    Synth* synth = Application_getInstance()->synth;

    for (MidiMessage* midiMessage = blockCurrent->midiMessageRoot; midiMessage; midiMessage = midiMessage->next) {
        if (midiMessage->time < startPosition) continue;
        float channel = 0;
        float velocity = midiMessage->velocity * EDIT_MODE_PLAYBACK_VELOCITY;
        float time = (midiMessage->time - startPosition) * blockTime;

        switch (midiMessage->type) {
            case FLUID_SEQ_NOTEON:
                Synth_sendNoteOn(synth, channel, midiMessage->pitch, velocity, time);
                break;
            case FLUID_SEQ_NOTEOFF:
                if (!self->ignoreNoteOff) {
                    Synth_sendNoteOff(synth, channel, midiMessage->pitch, time);
                }
                break;
        }
    }
    Synth_scheduleCallback(synth, blockTime * (1-startPosition));
    self->playStartTime = Synth_getTime(synth);
}


static void EditView_sequencerCallback(EditView* self) {
    if (self->playRepeat) {
        EditView_playBlock(self, 0.0f, true);
    }
    else {
        EditView_stopPlaying(self);
    }
}


static void EditView_stopPlaying(EditView* self) {
    Synth_noteOffAll(Application_getInstance()->synth);
    self->playStartTime = -1;
}


static bool EditView_isPlaying(EditView* self) {
    return self->playStartTime > 0;
}


static void EditView_setProgram(const char* const programName) {
    Synth_setProgramByName(Application_getInstance()->synth, 0, programName);
}


static void EditView_setTempo(EditView* self, int tempo) {
    self->tempo = tempo;
}


static void EditView_draw(EditView* self) {
    /* Vertical gridlines marking start of measures */
    for (int i = 0; i < BLOCK_MEASURES; i++) {
        EditView_drawItem(self, &(self->gridlinesVertical[i]), 0);
    }

    /* Horizontal gridlines for current key signature */
    for (int i = 0; i < OCTAVES*NOTES_IN_OCTAVE; i++) {
        int note = NOTES_IN_OCTAVE - i % NOTES_IN_OCTAVE - 1;
        KeySignature keySignature = self->score->keySignature;
        if (KEY_SIGNATURES[keySignature][note]) {
            EditView_drawItem(self, &(self->gridlinesHorizontal[i]), 0);
        }
    }

    /* Draw cursor */
    EditView_drawItem(self, &(self->cursor), CURSOR_SIZE_OFFSET);

    /* Draw notes */
    Renderer* renderer = Application_getInstance()->renderer;
    Block* blockCurrent = *Application_getInstance()->blockCurrent;
    MidiMessage* midiMessage = blockCurrent->midiMessageRoot;

    while (midiMessage) {
        if (midiMessage->type == FLUID_SEQ_NOTEON) {
            MidiMessage* midiMessageOther = midiMessage;
            while (midiMessageOther) {
                if (midiMessageOther->type == FLUID_SEQ_NOTEOFF && midiMessageOther->pitch == midiMessage->pitch) {
                    float viewportWidth = renderer->viewportWidth;
                    int iColumnStart = EditView_xCoordToColumnIndex(self, midiMessage->time * viewportWidth);
                    int iColumnEnd = EditView_xCoordToColumnIndex(self, midiMessageOther->time * viewportWidth);
                    int iRow = EditView_pitchToRowIndex(midiMessage->pitch);
                    Vector4 color = blockCurrent->color;

                    bool highlight;
                    if (EditView_isPlaying(self)) {
                        float time = Synth_getTime(Application_getInstance()->synth) - self->playStartTime;
                        float totalTime = 1000.0f * (float)(BLOCK_MEASURES * self->score->nBeatsPerMeasure * SECONDS_PER_MINUTE) / (float)self->tempo;
                        float progress = time / totalTime + self->playStartPosition;
                        float cursorX = Application_getInstance()->renderer->viewportWidth * progress;
                        int iCursorColumn = EditView_xCoordToColumnIndex(self, cursorX);
                        highlight = iColumnStart <= iCursorColumn && iColumnEnd > iCursorColumn;
                    }
                    else {
                        highlight = iRow == self->cursor.iRow && iColumnStart <= self->cursor.iColumn && iColumnEnd > self->cursor.iColumn;
                    }
                    if (highlight) {
                        color.x = HIGHLIGHT_STRENGTH * 1.0f + (1.0f - HIGHLIGHT_STRENGTH) * color.x;
                        color.y = HIGHLIGHT_STRENGTH * 1.0f + (1.0f - HIGHLIGHT_STRENGTH) * color.y;
                        color.z = HIGHLIGHT_STRENGTH * 1.0f + (1.0f - HIGHLIGHT_STRENGTH) * color.z;
                    }

                    GridItem item;
                    item.iRow = iRow;
                    item.iColumn = iColumnStart;
                    item.nRows = 1;
                    item.nColumns = iColumnEnd - iColumnStart;
                    item.color = color;
                    item.indicatorValue = midiMessage->velocity;
                    EditView_drawItem(self, &item, NOTE_SIZE_OFFSET);

                    break;
                }
                midiMessageOther = midiMessageOther->next;
            }
        }
        midiMessage = midiMessage->next;
    }

    /* Draw playback cursor */
    EditView_drawPlaybackCursor(self);
}


static void EditView_drawItem(EditView* self, GridItem* item, float offset) {
    float columnWidth = 2.0f/(BLOCK_MEASURES * self->score->nBeatsPerMeasure * MEASURE_RESOLUTION);
    float rowHeight = 2.0f/(OCTAVES * NOTES_IN_OCTAVE);

    float x1 = -1.0f + item->iColumn * columnWidth - offset;
    float x2 = -1.0f + item->iColumn * columnWidth + item->nColumns * columnWidth + offset;
    float y1 = -(-1.0f + item->iRow * rowHeight) + offset;
    float y2 = -(-1.0f + item->iRow * rowHeight + item->nRows * rowHeight) - offset;
    Renderer_drawQuad(Application_getInstance()->renderer, x1, x2, y1, y2, item->color);

    if (self->ctrlPressed && item->indicatorValue > 0.0f) {
        Vector4 indicatorColor = {1.0f - item->color.x, 1.0f - item->color.y, 1.0f - item->color.z, 1.0f};
        x1 = -1.0f + item->iColumn * columnWidth - offset;
        x2 = -1.0f + item->iColumn * columnWidth + columnWidth * VELOCITY_INDICATOR_WIDTH_EDIT_MODE + offset;
        y1 = -(-1.0f + (item->iRow + 1.0f - item->indicatorValue) * rowHeight) + offset;
        y2 = -(-1.0f + item->iRow * rowHeight + item->nRows * rowHeight) - offset;
        Renderer_drawQuad(Application_getInstance()->renderer, x1, x2, y1, y2, indicatorColor);
    }
}


static void EditView_drawPlaybackCursor(EditView* self) {
    if (!EditView_isPlaying(self)) return;

    float time = Synth_getTime(Application_getInstance()->synth) - self->playStartTime;
    float totalTime = 1000.0f * (float)(BLOCK_MEASURES * self->score->nBeatsPerMeasure * SECONDS_PER_MINUTE) / (float)self->tempo;
    float progress = time / totalTime + self->playStartPosition;
    float cursorX = -1.0f + 2.0f * progress;

    float x1 = cursorX - PLAYER_CURSOR_WIDTH / 2.0f;
    float x2 = cursorX + PLAYER_CURSOR_WIDTH / 2.0f;
    float y1 = -1.0f;
    float y2 = 1.0f;

    Renderer_drawQuad(Application_getInstance()->renderer, x1, x2, y1, y2, self->playbackCursorColor);
}


static bool EditView_updateCursorPosition(EditView* self, float x, float y) {
    int iColumnOld = self->cursor.iColumn;
    int iRowOld = self->cursor.iRow;

    self->cursor.iColumn = EditView_xCoordToColumnIndex(self, x);
    if (!self->midiMessageHeld) {
        self->cursor.iRow = EditView_yCoordToRowIndex(y);
    }

    int nColumns = BLOCK_MEASURES * self->score->nBeatsPerMeasure * MEASURE_RESOLUTION;
    int nRows = OCTAVES*NOTES_IN_OCTAVE;

    if (self->cursor.iColumn < 0) self->cursor.iColumn = 0;
    else if (self->cursor.iColumn > nColumns - 1) self->cursor.iColumn = nColumns - 1;
    if (self->cursor.iRow < 0) self->cursor.iRow = 0;
    else if (self->cursor.iRow > nRows - 1) self->cursor.iRow = nRows - 1;

    return self->cursor.iColumn == iColumnOld && self->cursor.iRow == iRowOld ? false : true;
}


static int EditView_rowIndexToNoteKey(int iRow) {
    return 95 - iRow;
}


static int EditView_pitchToRowIndex(int pitch) {
    return 95 - pitch;
}


static MidiMessage* EditView_addMidiMessage(int type, float time, int pitch, float velocity) {
    Block* blockCurrent = *Application_getInstance()->blockCurrent;
    MidiMessage* midiMessage = Block_addMidiMessage(blockCurrent, type, time, pitch, velocity);
    return midiMessage;
}


static void EditView_removeMidiMessage(MidiMessage* midiMessage) {
    Block_removeMidiMessage(midiMessage);
}


static int EditView_xCoordToColumnIndex(EditView* self, float x) {
    int nColumns = BLOCK_MEASURES * self->score->nBeatsPerMeasure * MEASURE_RESOLUTION;
    return (nColumns * x) / Application_getInstance()->renderer->viewportWidth;
}


static int EditView_yCoordToRowIndex(float y) {
    int nRows = OCTAVES*NOTES_IN_OCTAVE;
    return (nRows * y) / Application_getInstance()->renderer->viewportHeight;
}


static void EditView_toggleIgnoreNoteOff(EditView* self) {
    self->ignoreNoteOff = !self->ignoreNoteOff;
    printf("%s note off events for edit mode\n", self->ignoreNoteOff ? "Disabled" : "Enabled");
}
