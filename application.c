Application* Application_getInstance() {
    static Application* self = NULL;
    if (self) return self;

    self = ecalloc(1, sizeof(*self));
    self->state = EDIT_MODE;

    return self;
}

void Application_run() {
    Application* self = Application_getInstance();
    Synth_getInstance();
    while(Renderer_running()) {
        switch (self->state) {
            case OBJECT_MODE:
                break;
            case EDIT_MODE:
                Player_update();
                Canvas_draw();
                Player_drawCursor();
                break;
        }
        Renderer_updateScreen();
    }
}

void Application_switchState(void) {
    Application* self = Application_getInstance();
    self->state = !self->state;
    printf("Switched state to %s\n", self->state ? "edit mode" : "object mode");
}
