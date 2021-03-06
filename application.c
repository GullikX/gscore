/* Copyright (C) 2020-2021 Martin Gulliksson <martin@gullik.cc>
 *
 * This file is part of gscore.
 *
 * gscore is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 3.
 *
 * gscore is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>
 *
 */

static Application* _application = NULL;  /* Must only be accessed by Application_{new, free, getInstance} */


static Application* Application_new(const char* const filename) {
    Application* self = ecalloc(1, sizeof(*self));
    self->state = OBJECT_MODE;
    self->scoreCurrent = NULL;
    self->filename = filename;
    self->synth = Synth_new();

    if (fileExists(filename)) {
        printf("Loading file '%s'...\n", filename);
        self->scoreCurrent = Score_readFromFile(filename, self->synth);
    }
    else {
        printf("File '%s' does not exist, creating new empty score...\n", filename);
        self->scoreCurrent = Score_new(self->synth);
    }

    self->blockCurrent = &self->scoreCurrent->blocks[0];
    self->blockPrevious = NULL;

    self->editView = EditView_new(self->scoreCurrent);
    self->objectView = ObjectView_new(self->scoreCurrent);
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


static Application* Application_free(Application* self) {
    self->xevents = XEvents_free(self->xevents);
    self->renderer = Renderer_free(self->renderer);
    self->objectView = ObjectView_free(self->objectView);
    self->editView = EditView_free(self->editView);
    self->scoreCurrent = Score_free(self->scoreCurrent);
    self->synth = Synth_free(self->synth);
    free(self);
    _application = NULL;
    return NULL;
}


static Application* Application_getInstance(void) {
    if (!_application) die("Application instance not created yet");
    return _application;
}


static void Application_run(Application* self) {
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


static State Application_getState(Application* self) {
    return self->state;
}


static void Application_switchState(Application* self) {
    self->state = !self->state;
    printf("Switched state to %s\n", self->state ? "edit mode" : "object mode");
}


static void Application_switchBlock(Application* self, Block** block) {
    printf("Switching to block '%s'\n", (*block)->name);
    if (block != self->blockCurrent) {
        self->blockPrevious = self->blockCurrent;
        self->blockCurrent = block;
    }
}


static void Application_toggleBlock(Application* self) {
    if (!self->blockPrevious) return;
    Application_switchBlock(self, self->blockPrevious);
}


static void Application_writeScore(Application* self) {
    Score_writeToFile(self->scoreCurrent, self->filename);
}
