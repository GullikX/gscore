#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

void die(const char* const message) {
    printf("Error: %s\n", message);
    exit(EXIT_FAILURE);
}

int main() {
    if (!glfwInit()) {
        die("Failed to initialize GLFW");
    }

    GLFWwindow* window = glfwCreateWindow(640, 480, "GScore", NULL, NULL);
    if (!window) {
        die("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);
    printf("OpenGL %s\n", glGetString(GL_VERSION));

    if (glewInit() != GLEW_OK) {
        die("Failed to initialize GLEW");
    }

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GL_TRUE);
        }
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}
