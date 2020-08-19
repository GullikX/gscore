Canvas* Canvas_getInstance() {
    static Canvas* self = NULL;
    if (self) return self;

    self = ecalloc(1, sizeof(*self));
    self->noteIndex = 0;

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

    for (int i = 0; i < CANVAS_MAX_NOTES; i++) {
        self->notes[i].iRow = -1;
        self->notes[i].iColumn = -1;
        self->notes[i].nRows = 1;
        self->notes[i].nColumns = 1;
        self->notes[i].color = COLOR_NOTES;
    }

    self->cursor.iRow = 0;
    self->cursor.iColumn = 0;
    self->cursor.nRows = 1;
    self->cursor.nColumns = 1;
    self->cursor.color = COLOR_CURSOR;

    return self;
}


void Canvas_addNote() {
    /* TODO: Drag for longer note */
    Canvas* self = Canvas_getInstance();
    Synth_noteOn(Canvas_rowIndexToNoteKey(self->cursor.iRow));
    for (int i = 0; i < CANVAS_MAX_NOTES; i++) {
        if (self->notes[i].iRow == self->cursor.iRow && self->notes[i].iColumn == self->cursor.iColumn) {
            return;
        }
    }
    self->notes[self->noteIndex].iRow = self->cursor.iRow;
    self->notes[self->noteIndex].iColumn = self->cursor.iColumn;
    self->noteIndex++;
    if (self->noteIndex >= CANVAS_MAX_NOTES) {
        die("Too many notes");  /* TODO: Handle this better */
    }
}


void Canvas_releaseNote() {
    /* TODO: Stop dragging note */
    Synth_noteOffAll();
}


void Canvas_removeNote() {
    Canvas* self = Canvas_getInstance();
    for (int i = 0; i < CANVAS_MAX_NOTES; i++) {
        if (self->notes[i].iRow == self->cursor.iRow && self->notes[i].iColumn == self->cursor.iColumn) {
            self->notes[i].iRow = -1;
            self->notes[i].iColumn = -1;
            break;
        }
    }
}


void Canvas_draw() {
    Canvas* self = Canvas_getInstance();

    /* Vertical gridlines marking start of measures */
    for (int i = 0; i < BLOCK_MEASURES; i++) {
        Canvas_drawItem(&(self->gridlinesVertical[i]), 0);
    }

    /* Horizontal gridlines marking start of octaves */
    for (int i = 0; i < OCTAVES; i++) {
        Canvas_drawItem(&(self->gridlinesHorizontal[i]), 0);
    }

    /* Draw player cursor if playing */
    if (self->playerCursor.iColumn >= 0) {
        Canvas_drawItem(&(self->playerCursor), 0);
    }

    /* Draw notes */
    for (int i = 0; i < CANVAS_MAX_NOTES; i++) {
        if (self->notes[i].iRow >= 0 && self->notes[i].iColumn >= 0) {
            Canvas_drawItem(&(self->notes[i]), 0);
        }
    }

    /* Draw cursor */
    Canvas_drawItem(&(self->cursor), CURSOR_SIZE_OFFSET);
}


void Canvas_drawItem(CanvasItem* item, float offset) {
    float columnWidth = 2.0f/(BLOCK_MEASURES * MEASURE_RESOLUTION);
    float rowHeight = 2.0f/(OCTAVES * NOTES_IN_OCTAVE);

    float x1 = -1.0f + item->iColumn * columnWidth;
    float x2 = -1.0f + item->iColumn * columnWidth + item->nColumns * columnWidth;
    float y1 = -(-1.0f + item->iRow * rowHeight);
    float y2 = -(-1.0f + item->iRow * rowHeight + item->nRows * rowHeight);

    Vector2 positions[4];
    positions[0].x = x1 - offset; positions[0].y = y1 + offset;
    positions[1].x = x1 - offset; positions[1].y = y2 - offset;
    positions[2].x = x2 + offset; positions[2].y = y2 - offset;
    positions[3].x = x2 + offset; positions[3].y = y1 + offset;

    Quad quad;
    for (int i = 0; i < 4; i++) {
        quad.vertices[i].position = positions[i];
        quad.vertices[i].color = item->color;
    }
    Renderer_enqueueDraw(&quad);
}


void Canvas_updateCursorPosition(float x, float y) {
    Canvas* self = Canvas_getInstance();

    self->cursor.iColumn = Renderer_xCoordToColumnIndex(x);
    self->cursor.iRow = Renderer_yCoordToRowIndex(y);
}


void Canvas_updatePlayerCursorPosition(float x) {
    Canvas* self = Canvas_getInstance();
    self->playerCursor.iColumn = Renderer_xCoordToColumnIndex(x);
}


void Canvas_resetPlayerCursorPosition() {
    Canvas* self = Canvas_getInstance();
    self->playerCursor.iColumn = -1;
}


int Canvas_rowIndexToNoteKey(int iRow) {
    return 95-iRow;
}
