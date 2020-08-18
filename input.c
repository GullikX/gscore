void Input_processInput() {
    GLFWwindow* window = Renderer_getInstance()->window;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        Renderer_stop();
    }
}
