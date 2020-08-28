Canvas* Canvas_getInstance(void) {
    static Canvas* self = NULL;
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

    self->playerCursor.iRow = 0;
    self->playerCursor.iColumn = -1;
    self->playerCursor.nRows = nRows;
    self->playerCursor.nColumns = 1;
    self->playerCursor.color = COLOR_CURSOR;

    self->midiMessageRoot = ecalloc(1, sizeof(MidiMessage));
    self->midiMessageRoot->type = FLUID_SEQ_NOTE;
    self->midiMessageRoot->time = -1.0f;
    self->midiMessageRoot->channel = -1;
    self->midiMessageRoot->pitch = -1;
    self->midiMessageRoot->velocity = -1;
    self->midiMessageRoot->next = NULL;
    self->midiMessageRoot->prev = NULL;

    self->midiMessageHeld = NULL;

    self->cursor.iRow = 0;
    self->cursor.iColumn = 0;
    self->cursor.nRows = 1;
    self->cursor.nColumns = 1;
    self->cursor.color = COLOR_CURSOR;

    return self;
}


void Canvas_previewNote(void) {
    Canvas* self = Canvas_getInstance();
    if (self->playerCursor.iColumn < 0) {
        Synth_noteOn(Canvas_rowIndexToNoteKey(self->cursor.iRow));
    }
}


void Canvas_addNote(void) {
    Canvas* self = Canvas_getInstance();
    int nColumns = BLOCK_MEASURES*MEASURE_RESOLUTION;
    int pitch = Canvas_rowIndexToNoteKey(self->cursor.iRow);
    int channel = 0;  /* TODO */
    int velocity = 100;  /* TODO */

    if (!Player_playing()) {
        Synth_noteOn(pitch);
    }

    float timeStart = (float)self->cursor.iColumn / (float)nColumns;
    Canvas_addMidiMessage(FLUID_SEQ_NOTEON, timeStart, channel, pitch, velocity);

    float timeEnd = (float)(self->cursor.iColumn + 1) / (float)nColumns;
    self->midiMessageHeld = Canvas_addMidiMessage(FLUID_SEQ_NOTEOFF, timeEnd, channel, pitch, velocity);
}


void Canvas_dragNote(void) {
    Canvas* self = Canvas_getInstance();
    if (!self->midiMessageHeld) return;
}


void Canvas_releaseNote(void) {
    Canvas* self = Canvas_getInstance();
    self->midiMessageHeld = NULL;
    if (!Player_playing()) {
        Synth_noteOffAll();
    }
}


void Canvas_removeNote(void) {
    /* TODO */
}


void Canvas_draw(void) {
    Canvas* self = Canvas_getInstance();

    /* Vertical gridlines marking start of measures */
    for (int i = 0; i < BLOCK_MEASURES; i++) {
        Canvas_drawItem(&(self->gridlinesVertical[i]), 0);
    }

    /* Horizontal gridlines marking start of octaves */
    for (int i = 0; i < OCTAVES; i++) {
        Canvas_drawItem(&(self->gridlinesHorizontal[i]), 0);
    }

    /* Draw notes */
    MidiMessage* midiMessage = self->midiMessageRoot;
    while (midiMessage) {
        if (midiMessage->type == FLUID_SEQ_NOTEON) {
            MidiMessage* midiMessageOther = midiMessage;
            while (midiMessageOther) {
                if (midiMessageOther->type == FLUID_SEQ_NOTEOFF && midiMessageOther->pitch == midiMessage->pitch) {
                    float viewportWidth = Renderer_getInstance()->viewportWidth;
                    int iColumnStart = Renderer_xCoordToColumnIndex(midiMessage->time * viewportWidth);
                    int iColumnEnd = Renderer_xCoordToColumnIndex(midiMessageOther->time * viewportWidth);
                    int iRow = Canvas_pitchToRowIndex(midiMessage->pitch);

                    CanvasItem item;
                    item.iRow = iRow;
                    item.iColumn = iColumnStart;
                    item.nRows = 1;
                    item.nColumns = iColumnEnd - iColumnStart;
                    item.color = COLOR_NOTES;
                    Canvas_drawItem(&item, 0.0f);

                    break;
                }
                midiMessageOther = midiMessageOther->next;
            }
        }
        midiMessage = midiMessage->next;
    }

    /* Draw cursor */
    Canvas_drawItem(&(self->cursor), CURSOR_SIZE_OFFSET);
}


void Canvas_drawItem(CanvasItem* item, float offset) {
    float columnWidth = 2.0f/(BLOCK_MEASURES * MEASURE_RESOLUTION);
    float rowHeight = 2.0f/(OCTAVES * NOTES_IN_OCTAVE);

    float x1 = -1.0f + item->iColumn * columnWidth - offset;
    float x2 = -1.0f + item->iColumn * columnWidth + item->nColumns * columnWidth + offset;
    float y1 = -(-1.0f + item->iRow * rowHeight) + offset;
    float y2 = -(-1.0f + item->iRow * rowHeight + item->nRows * rowHeight) - offset;

    Renderer_drawQuad(x1, x2, y1, y2, item->color);
}


bool Canvas_updateCursorPosition(float x, float y) {
    Canvas* self = Canvas_getInstance();

    int iColumnOld = self->cursor.iColumn;
    int iRowOld = self->cursor.iRow;

    self->cursor.iColumn = Renderer_xCoordToColumnIndex(x);
    if (!self->midiMessageHeld) {
        self->cursor.iRow = Renderer_yCoordToRowIndex(y);
    }

    int nColumns = BLOCK_MEASURES*MEASURE_RESOLUTION;
    int nRows = OCTAVES*NOTES_IN_OCTAVE;

    if (self->cursor.iColumn < 0) self->cursor.iColumn = 0;
    else if (self->cursor.iColumn > nColumns - 1) self->cursor.iColumn = nColumns - 1;
    if (self->cursor.iRow < 0) self->cursor.iRow = 0;
    else if (self->cursor.iRow > nRows - 1) self->cursor.iRow = nRows - 1;

    return self->cursor.iColumn == iColumnOld && self->cursor.iRow == iRowOld ? false : true;
}


void Canvas_updatePlayerCursorPosition(float progress) {
    /* TODO */
}


void Canvas_resetPlayerCursorPosition(void) {
    Canvas* self = Canvas_getInstance();
    self->playerCursor.iColumn = -1;
    Synth_noteOffAll();
}


int Canvas_rowIndexToNoteKey(int iRow) {
    return 95 - iRow;
}


int Canvas_pitchToRowIndex(int pitch) {
    return 95 - pitch;
}


MidiMessage* Canvas_addMidiMessage(int type, float time, int channel, int pitch, int velocity) {
    Canvas* self = Canvas_getInstance();
    MidiMessage* midiMessage = ecalloc(1, sizeof(MidiMessage));
    midiMessage->type = type;
    midiMessage->time = time;
    midiMessage->channel = channel;
    midiMessage->pitch = pitch;
    midiMessage->velocity = velocity;
    midiMessage->next = NULL;
    midiMessage->prev = NULL;

    MidiMessage* midiMessageOther = self->midiMessageRoot;
    while (midiMessageOther->next && midiMessageOther->next->time < midiMessage->time) {
        midiMessageOther = midiMessageOther->next;
    }
    midiMessage->next = midiMessageOther->next;
    midiMessage->prev = midiMessageOther;
    if (midiMessageOther->next) midiMessageOther->next->prev = midiMessage;
    midiMessageOther->next = midiMessage;

    return midiMessage;
}
