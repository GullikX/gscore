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

Application* _application = NULL;  /* Must only be accessed by Application_{new, free, getInstance} */


Application* Application_new(const char* const filename) {
    Application* self = ecalloc(1, sizeof(*self));
    self->state = OBJECT_MODE;
    self->scoreCurrent = NULL;
    self->filename = filename;
    self->scoreCurrent = FileReader_read(filename);

    if (!_application) {
        _application = self;
    }
    else {
        die("There can only be one application instance");
    }

    return self;
}


Application* Application_free(Application* self) {
    free(self);
    _application = NULL;
    return NULL;
}


Application* Application_getInstance(void) {
    if (!_application) die("Application instance not created yet");
    return _application;
}


void Application_run(Application* self) {
    Synth_getInstance();
    while(Renderer_running()) {
        switch (Application_getState(self)) {
            case OBJECT_MODE:
                ScorePlayer_update();
                ObjectView_draw();
                ScorePlayer_drawCursor();
                break;
            case EDIT_MODE:
                BlockPlayer_update();
                EditView_draw();
                BlockPlayer_drawCursor();
                break;
        }
        Renderer_updateScreen();
    }
}


State Application_getState(Application* self) {
    return self->state;
}


void Application_switchState(Application* self) {
    self->state = !self->state;
    printf("Switched state to %s\n", self->state ? "edit mode" : "object mode");
}


void Application_writeScore(Application* self) {
    FileWriter_write(self->scoreCurrent, self->filename);
}
