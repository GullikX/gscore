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

XEvents* XEvents_new(GLFWwindow* glfwWindow) {
    XEvents* self = ecalloc(1, sizeof(*self));

    self->x11Display = glfwGetX11Display();
    self->x11Window = glfwGetX11Window(glfwWindow);
    printf("x11Window id: %lu\n", self->x11Window);
    for (int i = 0; i < ATOM_COUNT; i++) {
        self->atoms[i] = XInternAtom(self->x11Display, ATOM_NAMES[i], false);
    }

    return self;
}


XEvents* XEvents_free(XEvents* self) {
    free(self);
    return NULL;
}


void XEvents_processXEvents(XEvents* self) {
    Application* application = Application_getInstance();
    for (unsigned long i = 0; i < ATOM_COUNT; i++) {
        unsigned char* propertyValue = NULL;
        Atom atomDummy;
        int intDummy;
        unsigned long longDummy;

        int result = XGetWindowProperty(
            self->x11Display,
            self->x11Window,
            self->atoms[i],
            0L,
            sizeof(Atom),
            true,
            XA_STRING,
            &atomDummy,
            &intDummy,
            &longDummy,
            &longDummy,
            &propertyValue
        );

        if (result == Success && propertyValue) {
            printf("Received: %s=%s\n", ATOM_NAMES[i], propertyValue);
            State state = Application_getState(application);
            if (state == OBJECT_MODE) {
                switch (i) {
                    case ATOM_BPM:;
                        int tempoBpm = atoi((char*)propertyValue);
                        if (tempoBpm > 0 && tempoBpm < TEMPO_BPM_MAX) {
                            printf("Setting BPM to %d\n", tempoBpm);
                            application->scoreCurrent->tempo = tempoBpm;
                        }
                        else {
                            printf("Invalid BPM value '%s'\n", propertyValue);
                        }
                        break;
                    case ATOM_SYNTH_PROGRAM:;
                        int program = Synth_instrumentNameToId(application->synth, (char*)propertyValue);
                        ScorePlayer_setProgram(application->objectView->player, program);
                }
            }
            else if (state == EDIT_MODE) {
                switch (i) {
                    case ATOM_BPM:;
                        int tempoBpm = atoi((char*)propertyValue);
                        if (tempoBpm > 0 && tempoBpm < TEMPO_BPM_MAX) {
                            printf("Setting BPM to %d\n", tempoBpm);
                            EditView_setTempo(application->editView, tempoBpm);
                        }
                        else {
                            printf("Invalid BPM value '%s'\n", propertyValue);
                        }
                        break;
                    case ATOM_SYNTH_PROGRAM:;
                        int program = Synth_instrumentNameToId(application->synth, (char*)propertyValue);
                        EditView_setProgram(program);
                }
                break;
            }
        }
        XFree(propertyValue);
    }
}
