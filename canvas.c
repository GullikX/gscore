Canvas* Canvas_getInstance() {
    static Canvas* self = NULL;
    if (self) return self;

    self = ecalloc(1, sizeof(*self));
    for (int i = 0; i < CANVAS_MAX_NOTES; i++) {
        self->notes[i] = NULL;
    }
    self->noteIndex = 0;

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
    for (int i = 0; i < CANVAS_MAX_NOTES; i++) {
        Note_draw(self->notes[i]);
    }
}
