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

EditView* EditView_new(void) {
    EditView* self = ecalloc(1, sizeof(*self));

    int nRows = OCTAVES*NOTES_IN_OCTAVE;
    int nColumns = BLOCK_MEASURES*MEASURE_RESOLUTION;

    for (int i = 0; i < BLOCK_MEASURES; i++) {
        self->gridlinesVertical[i].iRow = 0;
        self->gridlinesVertical[i].iColumn = i*MEASURE_RESOLUTION;
        self->gridlinesVertical[i].nRows = nRows;
        self->gridlinesVertical[i].nColumns = 1;
        self->gridlinesVertical[i].color = COLOR_GRIDLINES;
    }

    for (int i = 0; i < OCTAVES; i++) {
        self->gridlinesHorizontal[i].iRow = (i+1)*NOTES_IN_OCTAVE - 1;
        self->gridlinesHorizontal[i].iColumn = 0;
        self->gridlinesHorizontal[i].nRows = 1;
        self->gridlinesHorizontal[i].nColumns = nColumns;
        self->gridlinesHorizontal[i].color = COLOR_GRIDLINES;
    }

    self->cursor.iRow = 0;
    self->cursor.iColumn = 0;
    self->cursor.nRows = 1;
    self->cursor.nColumns = 1;
    self->cursor.color = COLOR_CURSOR;

    self->player = BlockPlayer_new();

    return self;
}


EditView* EditView_free(EditView* self) {
    free(self);
    return NULL;
}


void EditView_update(EditView* self) {
    BlockPlayer_update(self->player);
}


void EditView_previewNote(EditView* self) {
    if (!BlockPlayer_playing(self->player)) {
        Synth_noteOn(Application_getInstance()->synth, EditView_rowIndexToNoteKey(self->cursor.iRow));
    }
}


void EditView_addNote(EditView* self) {
    if (BlockPlayer_playing(self->player)) return; /* TODO: allow this */
    int nColumns = BLOCK_MEASURES*MEASURE_RESOLUTION;
    int pitch = EditView_rowIndexToNoteKey(self->cursor.iRow);
    float velocity = DEFAULT_VELOCITY;

    if (!BlockPlayer_playing(self->player)) {
        Synth_noteOn(Application_getInstance()->synth, pitch);
    }

    float timeStart = (float)self->cursor.iColumn / (float)nColumns;
    EditView_addMidiMessage(FLUID_SEQ_NOTEON, timeStart, pitch, velocity);

    float timeEnd = (float)(self->cursor.iColumn + 1) / (float)nColumns;
    self->midiMessageHeld = EditView_addMidiMessage(FLUID_SEQ_NOTEOFF, timeEnd, pitch, 0);
}


void EditView_dragNote(EditView* self) {
    if (BlockPlayer_playing(self->player)) return; /* TODO: allow this */
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
    if (!BlockPlayer_playing(self->player)) {
        Synth_noteOffAll(Application_getInstance()->synth);
    }
}


void EditView_removeNote(EditView* self) {
    if (BlockPlayer_playing(self->player)) return; /* TODO: allow this */
    int nColumns = BLOCK_MEASURES*MEASURE_RESOLUTION;
    float time = (float)self->cursor.iColumn / (float)nColumns;
    int pitch = EditView_rowIndexToNoteKey(self->cursor.iRow);

    Application* application = Application_getInstance();
    MidiMessage* midiMessage = application->blockCurrent->midiMessageRoot;

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
    Application* application = Application_getInstance();
    MidiMessage* midiMessage = application->blockCurrent->midiMessageRoot;

    while (midiMessage) {
        if (midiMessage->type == FLUID_SEQ_NOTEON) {
            MidiMessage* midiMessageOther = midiMessage;
            while (midiMessageOther) {
                if (midiMessageOther->type == FLUID_SEQ_NOTEOFF && midiMessageOther->pitch == midiMessage->pitch) {
                    float viewportWidth = application->renderer->viewportWidth;
                    int iColumnStart = EditView_xCoordToColumnIndex(midiMessage->time * viewportWidth);
                    int iColumnEnd = EditView_xCoordToColumnIndex(midiMessageOther->time * viewportWidth);
                    int iRow = EditView_pitchToRowIndex(midiMessage->pitch);

                    GridItem item;
                    item.iRow = iRow;
                    item.iColumn = iColumnStart;
                    item.nRows = 1;
                    item.nColumns = iColumnEnd - iColumnStart;
                    item.color = application->blockCurrent->color;
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

    BlockPlayer_drawCursor(self->player);
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
    MidiMessage* midiMessage = ecalloc(1, sizeof(MidiMessage));
    midiMessage->type = type;
    midiMessage->time = time;
    midiMessage->pitch = pitch;
    midiMessage->velocity = velocity;
    midiMessage->next = NULL;
    midiMessage->prev = NULL;

    Application* application = Application_getInstance();
    MidiMessage* midiMessageOther = application->blockCurrent->midiMessageRoot;
    while (midiMessageOther->next && midiMessageOther->next->time < midiMessage->time) {
        midiMessageOther = midiMessageOther->next;
    }
    midiMessage->next = midiMessageOther->next;
    midiMessage->prev = midiMessageOther;
    if (midiMessageOther->next) midiMessageOther->next->prev = midiMessage;
    midiMessageOther->next = midiMessage;

    return midiMessage;
}


void EditView_removeMidiMessage(MidiMessage* midiMessage) {
    if (!midiMessage) return;
    midiMessage->prev->next = midiMessage->next;
    if (midiMessage->next) midiMessage->next->prev = midiMessage->prev;
    free(midiMessage);
}


int EditView_xCoordToColumnIndex(float x) {
    int nColumns = BLOCK_MEASURES*MEASURE_RESOLUTION;
    return (nColumns * x) / Application_getInstance()->renderer->viewportWidth;
}


int EditView_yCoordToRowIndex(float y) {
    int nRows = OCTAVES*NOTES_IN_OCTAVE;
    return (nRows * y) / Application_getInstance()->renderer->viewportHeight;
}
