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

void Input_setupCallbacks(GLFWwindow* window) {
    glfwSetKeyCallback(window, Input_keyCallback);
    glfwSetMouseButtonCallback(window, Input_mouseButtonCallback);
    glfwSetCursorPosCallback(window, Input_cursorPosCallback);
    glfwSetScrollCallback(window, Input_scrollCallback);
    glfwSetWindowSizeCallback(window, Input_windowSizeCallback);
}


void Input_keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
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


void Input_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
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


void Input_cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
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


void Input_scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    (void)window;
    printf("Mouse scroll: (%f, %f)\n", xoffset, yoffset);
}


void Input_windowSizeCallback(GLFWwindow* window, int width, int height) {
    (void)window;
    printf("Window size updated: (%d, %d)\n", width, height);
    Renderer_updateViewportSize(Application_getInstance()->renderer, width, height);
}


/* Object mode */
void Input_keyCallbackObjectMode(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window; (void)mods;
    Application* application = Application_getInstance();
    ObjectView* objectView = application->objectView;
    if (action == GLFW_PRESS) {
        const char* const keyName = glfwGetKeyName(key, scancode);
        if (keyName) {
            printf("Key pressed: %s\n", keyName);
            switch (*keyName) {
                case 'w':
                    Application_writeScore(application);
                    break;
                case 'q':
                    Renderer_stop(application->renderer);
                    break;
            }
        }
        else {
            printf("Key pressed: %d\n", key);
            switch (key) {
                case GLFW_KEY_TAB:
                    ScorePlayer_stop(objectView->player);
                    Application_switchState(application);
                    break;
                case GLFW_KEY_SPACE:
                    if (ScorePlayer_playing(objectView->player)) {
                        ScorePlayer_stop(objectView->player);
                    }
                    else {
                        ScorePlayer_playScore(objectView->player, application->scoreCurrent);
                    }
                    break;
            }
        }
    }
}


void Input_mouseButtonCallbackObjectMode(GLFWwindow* window, int button, int action, int mods) {
    Application* application = Application_getInstance();
    ObjectView* objectView = application->objectView;
    (void)window; (void)mods;
    if (action == GLFW_PRESS) {
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                puts("Left mouse button pressed!");
                ObjectView_addBlock(objectView);
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                puts("Right mouse button pressed!");
                ObjectView_removeBlock(objectView);
                break;
        }
    }
}


void Input_cursorPosCallbackObjectMode(GLFWwindow* window, double x, double y) {
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


/* Edit mode */
void Input_keyCallbackEditMode(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window;
    Application* application = Application_getInstance();
    EditView* editView = application->editView;
    bool modControl = mods & GLFW_MOD_CONTROL;
    bool modShift = mods & GLFW_MOD_SHIFT;
    if (action == GLFW_PRESS) {
        const char* const keyName = glfwGetKeyName(key, scancode);
        if (keyName) {
            printf("Key pressed: %s\n", keyName);
            switch (*keyName) {
                case 'i':
                    spawnSetXProp(ATOM_SYNTH_PROGRAM);
                    break;
                case 't':
                    spawnSetXProp(ATOM_BPM);
                    break;
                case 'w':
                    Application_writeScore(application);
                    break;
                case 'q':
                    Renderer_stop(application->renderer);
                    break;
            }
        }
        else {
            printf("Key pressed: %d\n", key);
            switch (key) {
                case GLFW_KEY_TAB:
                    BlockPlayer_stop(editView->player);
                    Application_switchState(application);
                    break;
                case GLFW_KEY_SPACE:
                    if (BlockPlayer_playing(editView->player)) {
                        BlockPlayer_stop(editView->player);
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
                        BlockPlayer_playBlock(editView->player, application->blockCurrent, startPosition, repeat);
                    }
                    break;
                case GLFW_KEY_ESCAPE:
                    BlockPlayer_stop(editView->player);
                    break;
            }
        }
    }
}


void Input_mouseButtonCallbackEditMode(GLFWwindow* window, int button, int action, int mods) {
    (void)window; (void)mods;
    Application* application = Application_getInstance();
    EditView* editView = application->editView;
    if (action == GLFW_PRESS) {
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                puts("Left mouse button pressed!");
                EditView_addNote(editView);
                break;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                puts("Middle mouse button pressed!");
                EditView_previewNote(editView);
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                puts("Right mouse button pressed!");
                EditView_removeNote(editView);
                break;
        }
    } else if (action == GLFW_RELEASE) {
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                puts("Left mouse button released!");
                EditView_releaseNote(editView);
                break;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                puts("Middle mouse button released!");
                EditView_releaseNote(editView);
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                puts("Right mouse button released!");
                break;
        }
    }
}


void Input_cursorPosCallbackEditMode(GLFWwindow* window, double x, double y) {
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
