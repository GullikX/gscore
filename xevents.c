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

static XEvents* XEvents_new(GLFWwindow* glfwWindow) {
    XEvents* self = ecalloc(1, sizeof(*self));

    self->x11Display = glfwGetX11Display();
    self->x11Window = glfwGetX11Window(glfwWindow);
    printf("x11Window id: %lu\n", self->x11Window);
    for (int i = 0; i < ATOM_COUNT; i++) {
        self->atoms[i] = XInternAtom(self->x11Display, ATOM_NAMES[i], false);
    }

    return self;
}


static XEvents* XEvents_free(XEvents* self) {
    free(self);
    return NULL;
}


static void XEvents_processXEvents(XEvents* self) {
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

        if (result == Success && propertyValue && strlen((char*)propertyValue)) {
            printf("Received: %s='%s'\n", ATOM_NAMES[i], propertyValue);
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
                            warn("Invalid BPM value '%s'", propertyValue);
                        }
                        break;
                    case ATOM_SYNTH_PROGRAM:
                        ObjectView_setProgram(application->objectView, (char*)propertyValue);
                        break;
                    case ATOM_SELECT_BLOCK:
                        Score_setBlockByName(application->scoreCurrent, (char*)propertyValue);
                        break;
                    case ATOM_RENAME_BLOCK:
                        Score_renameBlock(application->scoreCurrent, (char*)propertyValue);
                        break;
                    case ATOM_SET_BLOCK_COLOR:
                        Score_setBlockColor((char*)propertyValue);
                        break;
                    case ATOM_SET_TRACK_VELOCITY:;
                        float trackVelocity = atof((char*)propertyValue);
                        if (trackVelocity >= 0.0f && trackVelocity <= 1.0f) {
                            printf("Setting track velocity to to %f\n", trackVelocity);
                            ObjectView_setTrackVelocity(application->objectView, trackVelocity);
                        }
                        else {
                            warn("Invalid velocity value '%s', must be between %f and %f", propertyValue, VELOCITY_MIN, VELOCITY_MAX);
                        }
                        break;
                    case ATOM_QUIT:
                        if (!strcmp((char*)propertyValue, YES)) {
                            Renderer_stop(application->renderer);
                        }
                        break;
                }
            }
            else if (state == EDIT_MODE) {
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
                    case ATOM_SYNTH_PROGRAM:
                        EditView_setProgram((char*)propertyValue);
                        break;
                    case ATOM_SELECT_BLOCK:
                        Score_setBlockByName(application->scoreCurrent, (char*)propertyValue);
                        break;
                    case ATOM_RENAME_BLOCK:
                        Score_renameBlock(application->scoreCurrent, (char*)propertyValue);
                        break;
                    case ATOM_SET_BLOCK_COLOR:
                        Score_setBlockColor((char*)propertyValue);
                        break;
                    case ATOM_SET_KEY_SIGNATURE:
                        Score_setKeySignatureByName(application->scoreCurrent, (char*)propertyValue);
                        break;
                    case ATOM_QUIT:
                        if (!strcmp((char*)propertyValue, YES)) {
                            Renderer_stop(application->renderer);
                        }
                        break;
                }
                break;
            }
        }
        XFree(propertyValue);
    }
}
