Canvas* Canvas_getInstance() {
    static Canvas* self = NULL;
    if (self) return self;

    self = ecalloc(1, sizeof(*self));
    for (int i = 0; i < CANVAS_MAX_NOTES; i++) {
        self->notes[i] = NULL;
    }
    self->noteIndex = 0;

    for (int i = 0; i < BLOCK_MEASURES; i++) {
        self->gridlinesVertical[i].x1 = -1.0f + i * (2.0f/BLOCK_MEASURES);
        self->gridlinesVertical[i].x2 = -1.0f + i * (2.0f/BLOCK_MEASURES) + (2.0f/BLOCK_MEASURES) / MEASURE_RESOLUTION;
        self->gridlinesVertical[i].y1 = -1.0f;
        self->gridlinesVertical[i].y2 = 1.0f;
    }

    for (int i = 0; i < OCTAVES; i++) {
        self->gridlinesHorizontal[i].x1 = -1.0f;
        self->gridlinesHorizontal[i].x2 = 1.0f;
        self->gridlinesHorizontal[i].y1 = -1.0f + i * (2.0f/OCTAVES);
        self->gridlinesHorizontal[i].y2 = -1.0f + i * (2.0f/OCTAVES) + (2.0f/OCTAVES) / NOTES_IN_OCTAVE;
    }

    return self;
}


void Canvas_addNote(Note* note) {
    Canvas* self = Canvas_getInstance();
    self->notes[self->noteIndex] = note;
    self->noteIndex++;
    self->noteIndex %= CANVAS_MAX_NOTES;
}


void Canvas_draw() {
    Canvas* self = Canvas_getInstance();

    /* Vertical gridlines marking start of measures */
    for (int i = 0; i < BLOCK_MEASURES; i++) {
        Quad quad;
        quad.vertices[0].position.x = self->gridlinesVertical[i].x1; quad.vertices[0].position.y = self->gridlinesVertical[i].y1;
        quad.vertices[1].position.x = self->gridlinesVertical[i].x1; quad.vertices[1].position.y = self->gridlinesVertical[i].y2;
        quad.vertices[2].position.x = self->gridlinesVertical[i].x2; quad.vertices[2].position.y = self->gridlinesVertical[i].y2;
        quad.vertices[3].position.x = self->gridlinesVertical[i].x2; quad.vertices[3].position.y = self->gridlinesVertical[i].y1;
        for (int i = 0; i < 4; i++) {
            quad.vertices[i].color = COLOR_GRIDLINES;
        }
        Renderer_enqueueDraw(&quad);
    }

    /* Horizontal gridlines marking start of octaves */
    for (int i = 0; i < OCTAVES; i++) {
        Quad quad;
        quad.vertices[0].position.x = self->gridlinesHorizontal[i].x1; quad.vertices[0].position.y = self->gridlinesHorizontal[i].y1;
        quad.vertices[1].position.x = self->gridlinesHorizontal[i].x1; quad.vertices[1].position.y = self->gridlinesHorizontal[i].y2;
        quad.vertices[2].position.x = self->gridlinesHorizontal[i].x2; quad.vertices[2].position.y = self->gridlinesHorizontal[i].y2;
        quad.vertices[3].position.x = self->gridlinesHorizontal[i].x2; quad.vertices[3].position.y = self->gridlinesHorizontal[i].y1;
        for (int i = 0; i < 4; i++) {
            quad.vertices[i].color = COLOR_GRIDLINES;
        }
        Renderer_enqueueDraw(&quad);
    }

    /* Draw notes */
    for (int i = 0; i < CANVAS_MAX_NOTES; i++) {
        Note_draw(self->notes[i]);
    }

    /* Draw cursor */
    {
        Quad quad;
        quad.vertices[0].position.x = self->cursor.x1; quad.vertices[0].position.y = self->cursor.y1;
        quad.vertices[1].position.x = self->cursor.x1; quad.vertices[1].position.y = self->cursor.y2;
        quad.vertices[2].position.x = self->cursor.x2; quad.vertices[2].position.y = self->cursor.y2;
        quad.vertices[3].position.x = self->cursor.x2; quad.vertices[3].position.y = self->cursor.y1;
        for (int i = 0; i < 4; i++) {
            quad.vertices[i].color = COLOR_CURSOR;
        }
        Renderer_enqueueDraw(&quad);
    }
}


void Canvas_updateCursor(int rowIndex, int columnIndex) {
    Canvas* self = Canvas_getInstance();

    float columnWidth = (2.0f/BLOCK_MEASURES) / MEASURE_RESOLUTION;
    float rowHeight = (2.0f/OCTAVES) / NOTES_IN_OCTAVE;

    self->cursor.x1 = -1.0f + columnIndex * columnWidth;
    self->cursor.x2 = -1.0f + columnIndex * columnWidth + columnWidth;
    self->cursor.y1 = -(-1.0f + rowIndex * rowHeight);
    self->cursor.y2 = -(-1.0f + rowIndex * rowHeight + rowHeight);
}
