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
    switch (Application_getState()) {
        case OBJECT_MODE:
            Input_keyCallbackObjectMode(window, key, scancode, action, mods);
            break;
        case EDIT_MODE:
            Input_keyCallbackEditMode(window, key, scancode, action, mods);
            break;
    }
}


void Input_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    switch (Application_getState()) {
        case OBJECT_MODE:
            Input_mouseButtonCallbackObjectMode(window, button, action, mods);
            break;
        case EDIT_MODE:
            Input_mouseButtonCallbackEditMode(window, button, action, mods);
            break;
    }
}


void Input_cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    switch (Application_getState()) {
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
    Renderer_updateViewportSize(width, height);
}


/* Object mode */
void Input_keyCallbackObjectMode(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window; (void)mods;
    if (action == GLFW_PRESS) {
        const char* const keyName = glfwGetKeyName(key, scancode);
        if (keyName) {
            printf("Key pressed: %s\n", keyName);
            switch (*keyName) {
                case 'w':
                    Application_writeScore();
                    break;
                case 'q':
                    Renderer_stop();
                    break;
            }
        }
        else {
            printf("Key pressed: %d\n", key);
            switch (key) {
                case GLFW_KEY_TAB:
                    Application_switchState();
                    break;
                case GLFW_KEY_SPACE:
                    if (ScorePlayer_playing()) {
                        ScorePlayer_stop();
                    }
                    else {
                        ScorePlayer_playScore(Application_getInstance()->scoreCurrent);
                    }
                    break;
            }
        }
    }
}


void Input_mouseButtonCallbackObjectMode(GLFWwindow* window, int button, int action, int mods) {
    (void)window; (void)mods;
    if (action == GLFW_PRESS) {
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                puts("Left mouse button pressed!");
                ObjectView_addBlock();
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                puts("Right mouse button pressed!");
                ObjectView_removeBlock();
                break;
        }
    }
}


void Input_cursorPosCallbackObjectMode(GLFWwindow* window, double x, double y) {
    (void)window;
    bool updated = ObjectView_updateCursorPosition(x, y);
    if (updated) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            ObjectView_addBlock();
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            ObjectView_removeBlock();
        }
    }
}


/* Edit mode */
void Input_keyCallbackEditMode(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window;
    bool modControl = mods & GLFW_MOD_CONTROL;
    bool modShift = mods & GLFW_MOD_SHIFT;
    if (action == GLFW_PRESS) {
        const char* const keyName = glfwGetKeyName(key, scancode);
        if (keyName) {
            printf("Key pressed: %s\n", keyName);
            switch (*keyName) {
                case 'i':
                    spawnSetXProp(ATOM_SYNTH_PROGRAM);
                    return;
                case 't':
                    spawnSetXProp(ATOM_BPM);
                    return;
                case 'w':
                    Application_writeScore();
                    break;
                case 'q':
                    Renderer_stop();
                    return;
            }
        }
        else {
            printf("Key pressed: %d\n", key);
            switch (key) {
                case GLFW_KEY_TAB:
                    Application_switchState();
                    break;
                case GLFW_KEY_SPACE:
                    if (Player_playing()) {
                        Player_stop();
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
                        Player_playBlock(EditView_getInstance()->blockCurrent, startPosition, repeat);
                    }
                    return;
                case GLFW_KEY_ESCAPE:
                    Player_stop();
                    return;
            }
        }
    }
}


void Input_mouseButtonCallbackEditMode(GLFWwindow* window, int button, int action, int mods) {
    (void)window; (void)mods;
    if (action == GLFW_PRESS) {
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                puts("Left mouse button pressed!");
                EditView_addNote();
                break;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                puts("Middle mouse button pressed!");
                EditView_previewNote();
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                puts("Right mouse button pressed!");
                EditView_removeNote();
                break;
        }
    } else if (action == GLFW_RELEASE) {
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                puts("Left mouse button released!");
                EditView_releaseNote();
                break;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                puts("Middle mouse button released!");
                EditView_releaseNote();
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                puts("Right mouse button released!");
                break;
        }
    }
}


void Input_cursorPosCallbackEditMode(GLFWwindow* window, double x, double y) {
    (void)window;
    bool updated = EditView_updateCursorPosition(x, y);
    if (updated) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            EditView_dragNote();
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            EditView_removeNote();
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
            EditView_previewNote();
        }
    }
}
