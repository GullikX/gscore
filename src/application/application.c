/* Copyright (C) 2020-2024 Martin Gulliksson <martin@gullik.cc>
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

#include "application.h"

#include "common/constants/applicationstate.h"
#include "common/constants/input.h"
#include "common/score/score.h"
#include "common/structs/keyevent.h"
#include "common/util/alloc.h"
#include "common/util/inputmatcher.h"
#include "events/events.h"
#include "ui/editview/editview.h"
#include "ui/objectview/objectview.h"
#include "window/renderer.h"
#include "window/renderwindow.h"
#include "synth/synth.h"


struct Application {
    ApplicationState state;
    EditView* editView;
    ObjectView* objectView;
    RenderWindow* renderWindow;
    Renderer* renderer;
    Score* score;
    Synth* synth;
};


static void Application_onKeyEvent(Application* self, void* sender, KeyEvent* event);
static void Application_changeState(Application* self, ApplicationState state);


Application* Application_new(const char* const filename) {
    Application* self = ecalloc(1, sizeof(*self));

    Events_setup();

    self->score = Score_new(filename);
    self->renderWindow = RenderWindow_new();
    self->renderer = Renderer_new();
    self->editView = EditView_new(self->score);
    self->objectView = ObjectView_new(self->score);
    self->synth = Synth_new(self->score);

    Application_changeState(self, APPLICATION_STATE_OBJECT_MODE);

    Event_subscribe(EVENT_KEY_INPUT, self, EVENT_CALLBACK(Application_onKeyEvent), sizeof(KeyEvent));

    return self;
}


void Application_free(Application** pself) {
    Application* self = *pself;

    Event_unsubscribe(EVENT_KEY_INPUT, self, EVENT_CALLBACK(Application_onKeyEvent), sizeof(KeyEvent));

    Synth_free(&self->synth);
    ObjectView_free(&self->objectView);
    EditView_free(&self->editView);
    Renderer_free(&self->renderer);
    RenderWindow_free(&self->renderWindow);
    Score_free(&self->score);

    Events_teardown();

    sfree((void**)pself);
}


int Application_run(Application* self) {
    int exitCode = RenderWindow_run(self->renderWindow);
    return exitCode;
}


static void Application_onKeyEvent(Application* self, void* sender, KeyEvent* event) {
    (void)sender;

    KeyEvent* eventChangeApplicationState = &(KeyEvent){INPUT_KEY_TAB, INPUT_ACTION_PRESS, INPUT_NO_MODS};

    switch (self->state) {
        case APPLICATION_STATE_EDIT_MODE:;
            if(keyEventMatches(event, eventChangeApplicationState)) {
                Application_changeState(self, APPLICATION_STATE_OBJECT_MODE);
            }
            break;
        case APPLICATION_STATE_OBJECT_MODE:;
            if(keyEventMatches(event, eventChangeApplicationState)) {
                Application_changeState(self, APPLICATION_STATE_EDIT_MODE);
            }
            break;
        default:;
    }
}


static void Application_changeState(Application* self, ApplicationState state) {
    self->state = state;
    Event_post(self, EVENT_APPLICATION_STATE_CHANGED, &state, sizeof(ApplicationState));
}
