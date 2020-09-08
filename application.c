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

Application* _application = NULL;  /* Must only be accessed by Application_{new, free, getInstance} */


Application* Application_new(const char* const filename) {
    Application* self = ecalloc(1, sizeof(*self));
    self->state = OBJECT_MODE;
    self->scoreCurrent = NULL;
    self->filename = filename;

    if (fileExists(filename)) {
        printf("Loading file '%s'...\n", filename);
        self->scoreCurrent = Score_readFromFile(filename);
    }
    else {
        printf("File '%s' does not exist, creating new empty score...\n", filename);
        self->scoreCurrent = Score_new();
    }

    self->blockCurrent = &self->scoreCurrent->blocks[0];
    self->blockPrevious = NULL;

    self->editView = EditView_new(self->scoreCurrent);
    self->objectView = ObjectView_new(self->scoreCurrent);
    self->synth = Synth_new();
    self->renderer = Renderer_new();
    self->xevents = XEvents_new(self->renderer->window);

    if (!_application) {
        _application = self;
    }
    else {
        die("There can only be one application instance");
    }

    return self;
}


Application* Application_free(Application* self) {
    /* TODO: free all the things */
    self->editView = EditView_free(self->editView);
    self->objectView = ObjectView_free(self->objectView);
    self->synth = Synth_free(self->synth);
    self->xevents = XEvents_free(self->xevents);
    self->renderer = Renderer_free(self->renderer);
    free(self);
    _application = NULL;
    return NULL;
}


Application* Application_getInstance(void) {
    if (!_application) die("Application instance not created yet");
    return _application;
}


void Application_run(Application* self) {
    while(Renderer_running(self->renderer)) {
        switch (Application_getState(self)) {
            case OBJECT_MODE:
                ObjectView_draw(self->objectView);
                break;
            case EDIT_MODE:
                EditView_draw(self->editView);
                break;
        }
        Renderer_updateScreen(self->renderer);
        XEvents_processXEvents(self->xevents);
    }
}


State Application_getState(Application* self) {
    return self->state;
}


void Application_switchState(Application* self) {
    self->state = !self->state;
    printf("Switched state to %s\n", self->state ? "edit mode" : "object mode");
}


void Application_switchBlock(Application* self, Block** block) {
    printf("Switching to block '%s'\n", (*block)->name);
    self->blockPrevious = self->blockCurrent;
    self->blockCurrent = block;
}


void Application_toggleBlock(Application* self) {
    if (!self->blockPrevious) return;
    Application_switchBlock(self, self->blockPrevious);
}


void Application_writeScore(Application* self) {
    Score_writeToFile(self->scoreCurrent, self->filename);
}
