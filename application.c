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
                EditView_draw();
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
