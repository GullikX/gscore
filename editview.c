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

EditView* EditView_getInstance(void) {
    static EditView* self = NULL;
    if (self) return self;

    self = ecalloc(1, sizeof(*self));

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

    self->blockCurrent = ecalloc(1, sizeof(Block));

    self->blockCurrent->name = "blockName";
    self->blockCurrent->color = COLOR_NOTES;

    self->blockCurrent->midiMessageRoot = ecalloc(1, sizeof(MidiMessage));
    self->blockCurrent->midiMessageRoot->type = FLUID_SEQ_NOTE;
    self->blockCurrent->midiMessageRoot->time = -1.0f;
    self->blockCurrent->midiMessageRoot->channel = -1;
    self->blockCurrent->midiMessageRoot->pitch = -1;
    self->blockCurrent->midiMessageRoot->velocity = -1;
    self->blockCurrent->midiMessageRoot->next = NULL;
    self->blockCurrent->midiMessageRoot->prev = NULL;

    self->midiMessageHeld = NULL;

    self->cursor.iRow = 0;
    self->cursor.iColumn = 0;
    self->cursor.nRows = 1;
    self->cursor.nColumns = 1;
    self->cursor.color = COLOR_CURSOR;

    return self;
}


void EditView_previewNote(void) {
    EditView* self = EditView_getInstance();
    if (!Player_playing()) {
        Synth_noteOn(EditView_rowIndexToNoteKey(self->cursor.iRow));
    }
}


void EditView_addNote(void) {
    EditView* self = EditView_getInstance();
    int nColumns = BLOCK_MEASURES*MEASURE_RESOLUTION;
    int pitch = EditView_rowIndexToNoteKey(self->cursor.iRow);
    int channel = 0;  /* TODO */
    int velocity = 100;  /* TODO */

    if (!Player_playing()) {
        Synth_noteOn(pitch);
    }

    float timeStart = (float)self->cursor.iColumn / (float)nColumns;
    EditView_addMidiMessage(FLUID_SEQ_NOTEON, timeStart, channel, pitch, velocity);

    float timeEnd = (float)(self->cursor.iColumn + 1) / (float)nColumns;
    self->midiMessageHeld = EditView_addMidiMessage(FLUID_SEQ_NOTEOFF, timeEnd, channel, pitch, 0);
}


void EditView_dragNote(void) {
    EditView* self = EditView_getInstance();
    if (!self->midiMessageHeld) return;

    int nColumns = BLOCK_MEASURES*MEASURE_RESOLUTION;
    float time = (float)(self->cursor.iColumn + 1) / (float)nColumns;
    int channel = self->midiMessageHeld->channel;
    int pitch = self->midiMessageHeld->pitch;
    int velocity = self->midiMessageHeld->velocity;

    EditView_removeMidiMessage(self->midiMessageHeld);
    self->midiMessageHeld = EditView_addMidiMessage(FLUID_SEQ_NOTEOFF, time, channel, pitch, velocity);
}


void EditView_releaseNote(void) {
    EditView* self = EditView_getInstance();
    self->midiMessageHeld = NULL;
    if (!Player_playing()) {
        Synth_noteOffAll();
    }
}


void EditView_removeNote(void) {
    EditView* self = EditView_getInstance();
    int nColumns = BLOCK_MEASURES*MEASURE_RESOLUTION;
    float time = (float)self->cursor.iColumn / (float)nColumns;
    int pitch = EditView_rowIndexToNoteKey(self->cursor.iRow);

    MidiMessage* midiMessage = self->blockCurrent->midiMessageRoot;
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


void EditView_draw(void) {
    EditView* self = EditView_getInstance();

    /* Vertical gridlines marking start of measures */
    for (int i = 0; i < BLOCK_MEASURES; i++) {
        EditView_drawItem(&(self->gridlinesVertical[i]), 0);
    }

    /* Horizontal gridlines marking start of octaves */
    for (int i = 0; i < OCTAVES; i++) {
        EditView_drawItem(&(self->gridlinesHorizontal[i]), 0);
    }

    /* Draw notes */
    MidiMessage* midiMessage = self->blockCurrent->midiMessageRoot;
    while (midiMessage) {
        if (midiMessage->type == FLUID_SEQ_NOTEON) {
            MidiMessage* midiMessageOther = midiMessage;
            while (midiMessageOther) {
                if (midiMessageOther->type == FLUID_SEQ_NOTEOFF && midiMessageOther->pitch == midiMessage->pitch) {
                    float viewportWidth = Renderer_getInstance()->viewportWidth;
                    int iColumnStart = EditView_xCoordToColumnIndex(midiMessage->time * viewportWidth);
                    int iColumnEnd = EditView_xCoordToColumnIndex(midiMessageOther->time * viewportWidth);
                    int iRow = EditView_pitchToRowIndex(midiMessage->pitch);

                    GridItem item;
                    item.iRow = iRow;
                    item.iColumn = iColumnStart;
                    item.nRows = 1;
                    item.nColumns = iColumnEnd - iColumnStart;
                    item.color = COLOR_NOTES;
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
}


void EditView_drawItem(GridItem* item, float offset) {
    float columnWidth = 2.0f/(BLOCK_MEASURES * MEASURE_RESOLUTION);
    float rowHeight = 2.0f/(OCTAVES * NOTES_IN_OCTAVE);

    float x1 = -1.0f + item->iColumn * columnWidth - offset;
    float x2 = -1.0f + item->iColumn * columnWidth + item->nColumns * columnWidth + offset;
    float y1 = -(-1.0f + item->iRow * rowHeight) + offset;
    float y2 = -(-1.0f + item->iRow * rowHeight + item->nRows * rowHeight) - offset;

    Renderer_drawQuad(x1, x2, y1, y2, item->color);
}


bool EditView_updateCursorPosition(float x, float y) {
    EditView* self = EditView_getInstance();

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


MidiMessage* EditView_addMidiMessage(int type, float time, int channel, int pitch, int velocity) {
    EditView* self = EditView_getInstance();
    MidiMessage* midiMessage = ecalloc(1, sizeof(MidiMessage));
    midiMessage->type = type;
    midiMessage->time = time;
    midiMessage->channel = channel;
    midiMessage->pitch = pitch;
    midiMessage->velocity = velocity;
    midiMessage->next = NULL;
    midiMessage->prev = NULL;

    MidiMessage* midiMessageOther = self->blockCurrent->midiMessageRoot;
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
    return (nColumns * x) / Renderer_getInstance()->viewportWidth;
}


int EditView_yCoordToRowIndex(float y) {
    int nRows = OCTAVES*NOTES_IN_OCTAVE;
    return (nRows * y) / Renderer_getInstance()->viewportHeight;
}
