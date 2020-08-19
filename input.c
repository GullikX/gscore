void Input_setupCallbacks(GLFWwindow* window) {
    glfwSetKeyCallback(window, Input_keyCallback);
    glfwSetMouseButtonCallback(window, Input_mouseButtonCallback);
    glfwSetCursorPosCallback(window, Input_cursorPosCallback);
    glfwSetScrollCallback(window, Input_scrollCallback);
    glfwSetWindowSizeCallback(window, Input_windowSizeCallback);
}


void Input_keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window; (void)scancode; (void)action; (void)mods;
    if (action == GLFW_PRESS) {
        printf("Key pressed: %d\n", key);
        switch (key) {
            case GLFW_KEY_Q:
                Renderer_stop();
                break;
            case GLFW_KEY_SPACE:
                Player_toggle();
                break;
            case GLFW_KEY_ESCAPE:
                Player_stop();
                break;
        }
    }
}


void Input_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    (void)window; (void)mods;
    if (action == GLFW_PRESS) {
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                puts("Left mouse button pressed!");
                Canvas_addNote();
                break;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                puts("Middle mouse button pressed!");
                Canvas_previewNote();
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                puts("Right mouse button pressed!");
                Canvas_removeNote();
                break;
        }
    } else if (action == GLFW_RELEASE) {
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                puts("Left mouse button released!");
                Canvas_releaseNote();
                break;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                puts("Middle mouse button released!");
                Canvas_releaseNote();
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                puts("Right mouse button released!");
                break;
        }
    }
}


void Input_cursorPosCallback(GLFWwindow* window, double x, double y) {
    (void)window;
    bool updated = Canvas_updateCursorPosition(x, y);
    if (updated) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            Canvas_removeNote();
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
            Canvas_previewNote();
        }
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
