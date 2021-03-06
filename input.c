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

static void Input_setupCallbacks(GLFWwindow* window) {
    glfwSetKeyCallback(window, Input_keyCallback);
    glfwSetMouseButtonCallback(window, Input_mouseButtonCallback);
    glfwSetCursorPosCallback(window, Input_cursorPosCallback);
    glfwSetScrollCallback(window, Input_scrollCallback);
    glfwSetWindowSizeCallback(window, Input_windowSizeCallback);
}


static void Input_keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Application* application = Application_getInstance();
    switch (Application_getState(application)) {
        case OBJECT_MODE:
            Input_keyCallbackObjectMode(window, key, scancode, action, mods);
            break;
        case EDIT_MODE:
            Input_keyCallbackEditMode(window, key, scancode, action, mods);
            break;
    }
}


static void Input_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    Application* application = Application_getInstance();
    switch (Application_getState(application)) {
        case OBJECT_MODE:
            Input_mouseButtonCallbackObjectMode(window, button, action, mods);
            break;
        case EDIT_MODE:
            Input_mouseButtonCallbackEditMode(window, button, action, mods);
            break;
    }
}


static void Input_cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    Application* application = Application_getInstance();
    switch (Application_getState(application)) {
        case OBJECT_MODE:
            Input_cursorPosCallbackObjectMode(window, xpos, ypos);
            break;
        case EDIT_MODE:
            Input_cursorPosCallbackEditMode(window, xpos, ypos);
            break;
    }
}


static void Input_scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Application* application = Application_getInstance();
    switch (Application_getState(application)) {
        case OBJECT_MODE:
            Input_scrollCallbackObjectMode(window, xoffset, yoffset);
            break;
        case EDIT_MODE:
            Input_scrollCallbackEditMode(window, xoffset, yoffset);
            break;
    }
}


static void Input_windowSizeCallback(GLFWwindow* window, int width, int height) {
    (void)window;
    Renderer_updateViewportSize(Application_getInstance()->renderer, width, height);
}


/* Object mode */
static void Input_keyCallbackObjectMode(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window; (void)mods;
    Application* application = Application_getInstance();
    ObjectView* objectView = application->objectView;
    bool modControl = mods & GLFW_MOD_CONTROL;
    bool modShift = mods & GLFW_MOD_SHIFT;
    if (action == GLFW_PRESS) {
        const char* const keyName = glfwGetKeyName(key, scancode);
        if (keyName) {
            switch (*keyName) {
                case 'b':
                    if (!ObjectView_isPlaying(objectView)) {
                        spawnSetXProp(ATOM_SELECT_BLOCK);
                    }
                    break;
                case 'c':
                    if (!ObjectView_isPlaying(objectView)) {
                        spawnSetXProp(ATOM_SET_BLOCK_COLOR);
                    }
                    break;
                case 'i':
                    if (!ObjectView_isPlaying(objectView)) {
                        spawnSetXProp(ATOM_SYNTH_PROGRAM);
                    }
                    break;
                case 'n':
                    if (!ObjectView_isPlaying(objectView)) {
                        ObjectView_toggleIgnoreNoteOff(objectView);
                    }
                    break;
                case 'r':
                    if (!ObjectView_isPlaying(objectView)) {
                        spawnSetXProp(ATOM_RENAME_BLOCK);
                    }
                    break;
                case 't':
                    if (!ObjectView_isPlaying(objectView)) {
                        spawnSetXProp(ATOM_BPM);
                    }
                    break;
                case 'v':
                    if (!ObjectView_isPlaying(objectView)) {
                        spawnSetXProp(ATOM_SET_TRACK_VELOCITY);
                    }
                    break;
                case 'w':
                    Application_writeScore(application);
                    break;
                case 'q':
                    if (!ObjectView_isPlaying(objectView)) {
                        spawnSetXProp(ATOM_QUIT);
                    }
                    break;
            }
        }
        else {
            switch (key) {
                case GLFW_KEY_TAB:
                    if (modControl) {
                        Application_toggleBlock(application);
                    }
                    else {
                        ObjectView_stopPlaying(objectView);
                        Application_switchState(application);
                    }
                    break;
                case GLFW_KEY_LEFT_CONTROL:
                    ObjectView_setCtrlPressed(objectView, true);
                    break;
                case GLFW_KEY_SPACE:
                    if (ObjectView_isPlaying(objectView)) {
                        ObjectView_stopPlaying(objectView);
                    }
                    else {
                        bool repeat = modShift;
                        float startPosition = 0.0f;
                        if (modControl) {
                            double cursorX, cursorY;
                            glfwGetCursorPos(window, &cursorX, &cursorY);
                            int windowWidth, windowHeight;
                            glfwGetWindowSize(window, &windowWidth, &windowHeight);
                            startPosition = (float)cursorX / (float)windowWidth;
                        }
                        ObjectView_playScore(objectView, startPosition, repeat);
                    }
                    break;
                case GLFW_KEY_ESCAPE:
                    ObjectView_stopPlaying(objectView);
                    break;
                case GLFW_KEY_UP:
                    if (modControl) {
                        if (!ObjectView_isPlaying(objectView)) {
                            Score_removeTrack(application->scoreCurrent);
                        }
                    }
                    break;
                case GLFW_KEY_DOWN:
                    if (modControl) {
                        if (!ObjectView_isPlaying(objectView)) {
                            Score_addTrack(application->scoreCurrent);
                        }
                    }
                    break;
                case GLFW_KEY_LEFT:
                    if (modControl) {
                        if (!ObjectView_isPlaying(objectView)) {
                            Score_decreaseLength(application->scoreCurrent);
                        }
                    }
                    break;
                case GLFW_KEY_RIGHT:
                    if (modControl) {
                        if (!ObjectView_isPlaying(objectView)) {
                            Score_increaseLength(application->scoreCurrent);
                        }
                    }
                    break;
            }
        }
    }
    else if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_LEFT_CONTROL:
                ObjectView_setCtrlPressed(objectView, false);
                break;
        }
    }
}


static void Input_mouseButtonCallbackObjectMode(GLFWwindow* window, int button, int action, int mods) {
    Application* application = Application_getInstance();
    ObjectView* objectView = application->objectView;
    (void)window; (void)mods;
    if (action == GLFW_PRESS) {
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                ObjectView_addBlock(objectView);
                break;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                ObjectView_editBlock(objectView);
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                ObjectView_removeBlock(objectView);
                break;
        }
    }
}


static void Input_cursorPosCallbackObjectMode(GLFWwindow* window, double x, double y) {
    (void)window;
    Application* application = Application_getInstance();
    ObjectView* objectView = application->objectView;
    bool updated = ObjectView_updateCursorPosition(objectView, x, y);
    if (updated) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            ObjectView_addBlock(objectView);
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            ObjectView_removeBlock(objectView);
        }
    }
}


static void Input_scrollCallbackObjectMode(GLFWwindow* window, double xoffset, double yoffset) {
    (void)xoffset;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        Application* application = Application_getInstance();
        ObjectView* objectView = application->objectView;
        ObjectView_adjustBlockVelocity(objectView, yoffset * VELOCITY_ADJUSTMENT_AMOUNT);
    }
}


/* Edit mode */
static void Input_keyCallbackEditMode(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window;
    Application* application = Application_getInstance();
    EditView* editView = application->editView;
    bool modControl = mods & GLFW_MOD_CONTROL;
    bool modShift = mods & GLFW_MOD_SHIFT;
    if (action == GLFW_PRESS) {
        const char* const keyName = glfwGetKeyName(key, scancode);
        if (keyName) {
            switch (*keyName) {
                case 'b':
                    if (!EditView_isPlaying(editView)) {
                        spawnSetXProp(ATOM_SELECT_BLOCK);
                    }
                    break;
                case 'c':
                    if (!EditView_isPlaying(editView)) {
                        spawnSetXProp(ATOM_SET_BLOCK_COLOR);
                    }
                    break;
                case 'i':
                    if (!EditView_isPlaying(editView)) {
                        spawnSetXProp(ATOM_SYNTH_PROGRAM);
                    }
                    break;
                case 'k':
                    if (!EditView_isPlaying(editView)) {
                        spawnSetXProp(ATOM_SET_KEY_SIGNATURE);
                    }
                    break;
                case 'n':
                    if (!EditView_isPlaying(editView)) {
                        EditView_toggleIgnoreNoteOff(editView);
                    }
                    break;
                case 'r':
                    if (!EditView_isPlaying(editView)) {
                        spawnSetXProp(ATOM_RENAME_BLOCK);
                    }
                    break;
                case 't':
                    if (!EditView_isPlaying(editView)) {
                        spawnSetXProp(ATOM_BPM);
                    }
                    break;
                case 'w':
                    Application_writeScore(application);
                    break;
                case 'q':
                    if (!EditView_isPlaying(editView)) {
                        spawnSetXProp(ATOM_QUIT);
                    }
                    break;
            }
        }
        else {
            switch (key) {
                case GLFW_KEY_TAB:
                    EditView_stopPlaying(editView);
                    if (modControl) {
                        Application_toggleBlock(application);
                    }
                    else {
                        Application_switchState(application);
                    }
                    break;
                case GLFW_KEY_LEFT_CONTROL:
                    EditView_setCtrlPressed(editView, true);
                    break;
                case GLFW_KEY_SPACE:
                    if (EditView_isPlaying(editView)) {
                        EditView_stopPlaying(editView);
                    }
                    else {
                        bool repeat = modShift;
                        float startPosition = 0.0f;
                        if (modControl) {
                            double cursorX, cursorY;
                            glfwGetCursorPos(window, &cursorX, &cursorY);
                            int windowWidth, windowHeight;
                            glfwGetWindowSize(window, &windowWidth, &windowHeight);
                            startPosition = (float)cursorX / (float)windowWidth;
                        }
                        EditView_playBlock(editView, startPosition, repeat);
                    }
                    break;
                case GLFW_KEY_ESCAPE:
                    EditView_stopPlaying(editView);
                    break;
                case GLFW_KEY_UP:
                    if (modControl) {
                        EditView_transpose(editView, 1);
                    }
                    break;
                case GLFW_KEY_DOWN:
                    if (modControl) {
                        EditView_transpose(editView, -1);
                    }
                    break;
            }
        }
    }
    else if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_LEFT_CONTROL:
                EditView_setCtrlPressed(editView, false);
                break;
        }
    }
}


static void Input_mouseButtonCallbackEditMode(GLFWwindow* window, int button, int action, int mods) {
    (void)window; (void)mods;
    Application* application = Application_getInstance();
    EditView* editView = application->editView;
    if (action == GLFW_PRESS) {
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                EditView_addNote(editView);
                break;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                EditView_previewNote(editView);
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                EditView_removeNote(editView);
                break;
        }
    } else if (action == GLFW_RELEASE) {
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                EditView_releaseNote(editView);
                break;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                EditView_releaseNote(editView);
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                break;
        }
    }
}


static void Input_cursorPosCallbackEditMode(GLFWwindow* window, double x, double y) {
    (void)window;
    Application* application = Application_getInstance();
    EditView* editView = application->editView;
    bool updated = EditView_updateCursorPosition(editView, x, y);
    if (updated) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            EditView_dragNote(editView);
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            EditView_removeNote(editView);
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
            EditView_previewNote(editView);
        }
    }
}


static void Input_scrollCallbackEditMode(GLFWwindow* window, double xoffset, double yoffset) {
    (void)xoffset;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        Application* application = Application_getInstance();
        EditView* editView = application->editView;
        EditView_adjustNoteVelocity(editView, yoffset * VELOCITY_ADJUSTMENT_AMOUNT);
    }
}
