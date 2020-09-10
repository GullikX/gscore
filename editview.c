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

EditView* EditView_new(Score* score) {
    EditView* self = ecalloc(1, sizeof(*self));

    int nRows = OCTAVES*NOTES_IN_OCTAVE;
    int nColumns = BLOCK_MEASURES*MEASURE_RESOLUTION;

    for (int i = 0; i < BLOCK_MEASURES; i++) {
        self->gridlinesVertical[i].iRow = 0;
        self->gridlinesVertical[i].iColumn = i*MEASURE_RESOLUTION;
        self->gridlinesVertical[i].nRows = nRows;
        self->gridlinesVertical[i].nColumns = 1;
        bool success = hexColorToRgb(COLOR_GRIDLINES, &self->gridlinesVertical[i].color);
        if (!success) die("Invalid gridline color");
    }

    for (int i = 0; i < OCTAVES; i++) {
        self->gridlinesHorizontal[i].iRow = (i+1)*NOTES_IN_OCTAVE - 1;
        self->gridlinesHorizontal[i].iColumn = 0;
        self->gridlinesHorizontal[i].nRows = 1;
        self->gridlinesHorizontal[i].nColumns = nColumns;
        bool success = hexColorToRgb(COLOR_GRIDLINES, &self->gridlinesHorizontal[i].color);
        if (!success) die("Invalid gridline color");
    }

    self->cursor.iRow = 0;
    self->cursor.iColumn = 0;
    self->cursor.nRows = 1;
    self->cursor.nColumns = 1;
    bool success = hexColorToRgb(COLOR_CURSOR, &self->cursor.color);
    if (!success) die("Invalid cursor color");

    self->playStartTime = -1;
    self->tempo = score->tempo;
    success = hexColorToRgb(COLOR_CURSOR, &self->playbackCursorColor);
    if (!success) die("Invalid playback cursor color");

    self->ignoreNoteOff = IGNORE_NOTE_OFF_DEFAULT;

    return self;
}


EditView* EditView_free(EditView* self) {
    free(self);
    return NULL;
}


void EditView_previewNote(EditView* self) {
    if (!EditView_isPlaying(self)) {
        Synth_noteOn(Application_getInstance()->synth, EditView_rowIndexToNoteKey(self->cursor.iRow));
    }
}


void EditView_addNote(EditView* self) {
    if (EditView_isPlaying(self)) return; /* TODO: allow this */
    int nColumns = BLOCK_MEASURES*MEASURE_RESOLUTION;
    int pitch = EditView_rowIndexToNoteKey(self->cursor.iRow);
    float velocity = DEFAULT_VELOCITY;

    Synth_noteOn(Application_getInstance()->synth, pitch);

    float timeStart = (float)self->cursor.iColumn / (float)nColumns;
    EditView_addMidiMessage(FLUID_SEQ_NOTEON, timeStart, pitch, velocity);

    float timeEnd = (float)(self->cursor.iColumn + 1) / (float)nColumns;
    self->midiMessageHeld = EditView_addMidiMessage(FLUID_SEQ_NOTEOFF, timeEnd, pitch, 0);
}


void EditView_dragNote(EditView* self) {
    if (EditView_isPlaying(self)) return; /* TODO: allow this */
    if (!self->midiMessageHeld) return;

    int nColumns = BLOCK_MEASURES*MEASURE_RESOLUTION;
    float time = (float)(self->cursor.iColumn + 1) / (float)nColumns;
    int pitch = self->midiMessageHeld->pitch;
    float velocity = self->midiMessageHeld->velocity;

    EditView_removeMidiMessage(self->midiMessageHeld);
    self->midiMessageHeld = EditView_addMidiMessage(FLUID_SEQ_NOTEOFF, time, pitch, velocity);
}


void EditView_releaseNote(EditView* self) {
    self->midiMessageHeld = NULL;
    if (!EditView_isPlaying(self)) {
        Synth_noteOffAll(Application_getInstance()->synth);
    }
}


void EditView_removeNote(EditView* self) {
    if (EditView_isPlaying(self)) return; /* TODO: allow this */
    int nColumns = BLOCK_MEASURES*MEASURE_RESOLUTION;
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


void EditView_playBlock(EditView* self, float startPosition, bool repeat) {
    (void)startPosition; (void)repeat; /* TODO */
    float blockTime = (float)(BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE) / (float)self->tempo;
    Block* blockCurrent = *Application_getInstance()->blockCurrent;
    Synth* synth = Application_getInstance()->synth;
    float stopTime = -1.0f;

    for (MidiMessage* midiMessage = blockCurrent->midiMessageRoot; midiMessage; midiMessage = midiMessage->next) {
        if (midiMessage->time < 0) continue;
        float channel = 0;
        float velocity = midiMessage->velocity * EDIT_MODE_PLAYBACK_VELOCITY;
        float time = midiMessage->time * blockTime;
        if (stopTime < time) stopTime = time;

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
    Synth_scheduleCallback(synth, stopTime);
    self->playStartTime = Synth_getTime(synth);
}


void EditView_stopPlaying(EditView* self) {
    Synth_noteOffAll(Application_getInstance()->synth);
    self->playStartTime = -1;
}


bool EditView_isPlaying(EditView* self) {
    return self->playStartTime > 0;
}


void EditView_setProgram(const char* const programName) {
    Synth_setProgramByName(Application_getInstance()->synth, 0, programName);
}


void EditView_setTempo(EditView* self, int tempo) {
    self->tempo = tempo;
}


char* EditView_getTempoString(void) {  /* called from input callback (no instance reference) */
    EditView* self = Application_getInstance()->editView;
    snprintf(self->tempoString, 64, "%d", self->tempo);
    return self->tempoString;
}


void EditView_draw(EditView* self) {
    /* Vertical gridlines marking start of measures */
    for (int i = 0; i < BLOCK_MEASURES; i++) {
        EditView_drawItem(&(self->gridlinesVertical[i]), 0);
    }

    /* Horizontal gridlines marking start of octaves */
    for (int i = 0; i < OCTAVES; i++) {
        EditView_drawItem(&(self->gridlinesHorizontal[i]), 0);
    }

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
                    int iColumnStart = EditView_xCoordToColumnIndex(midiMessage->time * viewportWidth);
                    int iColumnEnd = EditView_xCoordToColumnIndex(midiMessageOther->time * viewportWidth);
                    int iRow = EditView_pitchToRowIndex(midiMessage->pitch);

                    GridItem item;
                    item.iRow = iRow;
                    item.iColumn = iColumnStart;
                    item.nRows = 1;
                    item.nColumns = iColumnEnd - iColumnStart;
                    item.color = blockCurrent->color;
                    EditView_drawItem(&item, NOTE_SIZE_OFFSET);

                    break;
                }
                midiMessageOther = midiMessageOther->next;
            }
        }
        midiMessage = midiMessage->next;
    }

    /* Draw cursor */
    EditView_drawItem(&(self->cursor), CURSOR_SIZE_OFFSET);

    /* Draw playback cursor */
    EditView_drawPlaybackCursor(self);
}


void EditView_drawItem(GridItem* item, float offset) {
    float columnWidth = 2.0f/(BLOCK_MEASURES * MEASURE_RESOLUTION);
    float rowHeight = 2.0f/(OCTAVES * NOTES_IN_OCTAVE);

    float x1 = -1.0f + item->iColumn * columnWidth - offset;
    float x2 = -1.0f + item->iColumn * columnWidth + item->nColumns * columnWidth + offset;
    float y1 = -(-1.0f + item->iRow * rowHeight) + offset;
    float y2 = -(-1.0f + item->iRow * rowHeight + item->nRows * rowHeight) - offset;

    Renderer_drawQuad(Application_getInstance()->renderer, x1, x2, y1, y2, item->color);
}


void EditView_drawPlaybackCursor(EditView* self) {
    if (!EditView_isPlaying(self)) return;

    float time = Synth_getTime(Application_getInstance()->synth) - self->playStartTime;
    float totalTime = 1000.0f * (float)(BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE) / (float)self->tempo;
    float progress = time / totalTime;
    float cursorX = -1.0f + 2.0f * progress;

    float x1 = cursorX - PLAYER_CURSOR_WIDTH / 2.0f;
    float x2 = cursorX + PLAYER_CURSOR_WIDTH / 2.0f;
    float y1 = -1.0f;
    float y2 = 1.0f;

    Renderer_drawQuad(Application_getInstance()->renderer, x1, x2, y1, y2, self->playbackCursorColor);
}


bool EditView_updateCursorPosition(EditView* self, float x, float y) {
    int iColumnOld = self->cursor.iColumn;
    int iRowOld = self->cursor.iRow;

    self->cursor.iColumn = EditView_xCoordToColumnIndex(x);
    if (!self->midiMessageHeld) {
        self->cursor.iRow = EditView_yCoordToRowIndex(y);
    }

    int nColumns = BLOCK_MEASURES*MEASURE_RESOLUTION;
    int nRows = OCTAVES*NOTES_IN_OCTAVE;

    if (self->cursor.iColumn < 0) self->cursor.iColumn = 0;
    else if (self->cursor.iColumn > nColumns - 1) self->cursor.iColumn = nColumns - 1;
    if (self->cursor.iRow < 0) self->cursor.iRow = 0;
    else if (self->cursor.iRow > nRows - 1) self->cursor.iRow = nRows - 1;

    return self->cursor.iColumn == iColumnOld && self->cursor.iRow == iRowOld ? false : true;
}


int EditView_rowIndexToNoteKey(int iRow) {
    return 95 - iRow;
}


int EditView_pitchToRowIndex(int pitch) {
    return 95 - pitch;
}


MidiMessage* EditView_addMidiMessage(int type, float time, int pitch, float velocity) {
    Block* blockCurrent = *Application_getInstance()->blockCurrent;
    MidiMessage* midiMessage = Block_addMidiMessage(blockCurrent, type, time, pitch, velocity);
    return midiMessage;
}


void EditView_removeMidiMessage(MidiMessage* midiMessage) {
    Block_removeMidiMessage(midiMessage);
}


int EditView_xCoordToColumnIndex(float x) {
    int nColumns = BLOCK_MEASURES*MEASURE_RESOLUTION;
    return (nColumns * x) / Application_getInstance()->renderer->viewportWidth;
}


int EditView_yCoordToRowIndex(float y) {
    int nRows = OCTAVES*NOTES_IN_OCTAVE;
    return (nRows * y) / Application_getInstance()->renderer->viewportHeight;
}


void EditView_toggleIgnoreNoteOff(EditView* self) {
    self->ignoreNoteOff = !self->ignoreNoteOff;
    printf("%s note off events for edit mode\n", self->ignoreNoteOff ? "Disabled" : "Enabled");
}
