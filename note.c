Note* Note_new(int start, int end, int pitch, int velocity) {
    Note* self = ecalloc(1, sizeof(*self));
    self->start = start;
    self->end = end;
    self->pitch = pitch;
    self->velocity = velocity;
    return self;
}


void Note_draw(Note* self) {
    if (!self) return;
    Quad quad;
    quad.vertices[0].position.x = 0.25f; quad.vertices[0].position.y = 0.25f;
    quad.vertices[1].position.x = 0.25f; quad.vertices[1].position.y = 0.75f;
    quad.vertices[2].position.x = 0.75f; quad.vertices[2].position.y = 0.75f;
    quad.vertices[3].position.x = 0.75f; quad.vertices[3].position.y = 0.25f;
    for (int i = 0; i < 4; i++) {
        quad.vertices[i].color = COLOR_1;
    }
    Renderer_enqueueDraw(&quad);
}
