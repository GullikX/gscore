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
    }
    if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
        Renderer_stop();
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
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                puts("Right mouse button pressed!");
                Canvas_removeNote();
                break;
        }
    }
}


void Input_cursorPosCallback(GLFWwindow* window, double x, double y) {
    (void)window;
    Canvas_updateCursorPosition(x, y);
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
