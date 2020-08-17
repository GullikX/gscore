#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>


const char* const vertexShaderSource =
    "#version 120\n"
    "attribute vec4 color;"
    "void main() {"
    "   gl_FrontColor = color;"
    "   gl_Position = gl_Vertex;"
    "}";


const char* const fragmentShaderSource =
    "#version 120\n"
    "void main() {"
    "    gl_FragColor = gl_Color;"
    "}";


void die(const char* const message) {
    printf("Error: %s\n", message);
    exit(EXIT_FAILURE);
}


void* ecalloc(size_t nItems, size_t itemSize) {
    void* pointer = calloc(nItems, itemSize);
    if (!pointer) {
        die("Failed to allocate memory");
    }
    return pointer;
}


GLuint createShader(const GLenum type, const char* const shaderSource) {
    const char* const typeString = type == GL_VERTEX_SHADER ? "vertex" : "fragment";
    GLuint shaderId = glCreateShader(type);
    glShaderSource(shaderId, 1, &shaderSource, NULL);
    glCompileShader(shaderId);

    int compilationResult = -1;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compilationResult);
    if (!compilationResult) {
        int messageLength = -1;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &messageLength);
        char* const message = ecalloc(messageLength, sizeof(char));
        glGetShaderInfoLog(shaderId, messageLength, &messageLength, message);
        printf("Failed to compile %s shader! Message:\n    %s\n", typeString, message);
        free(message);
        die("Failed to compile shader");
    }
    return shaderId;
}


GLuint createProgram() {
    GLuint programId = glCreateProgram();

    GLuint vertexShaderId = createShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShaderId = createShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);
    glValidateProgram(programId);

    glDeleteShader(vertexShaderId);
    glDeleteShader(fragmentShaderId);

    return programId;
}


void windowSizeCallback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}


int main() {
    if (!glfwInit()) {
        die("Failed to initialize GLFW");
    }

    GLFWwindow* window = glfwCreateWindow(640, 640, "GScore", NULL, NULL);
    if (!window) {
        die("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, windowSizeCallback);
    printf("OpenGL %s\n", glGetString(GL_VERSION));

    if (glewInit() != GLEW_OK) {
        die("Failed to initialize GLEW");
    }

    GLuint programId = createProgram();
    glUseProgram(programId);

    float vertices[] = {
        // Positions           // Colors (RGBA)
         0.25f,  0.25f,        0.0f, 0.0f, 1.0f, 1.0f,
         0.25f,  0.75f,        1.0f, 0.0f, 0.0f, 1.0f,
         0.75f,  0.75f,        1.0f, 0.0f, 0.0f, 1.0f,
         0.75f,  0.25f,        1.0f, 0.0f, 0.0f, 1.0f,

        -0.25f, -0.25f,        1.0f, 1.0f, 0.0f, 1.0f,
        -0.25f, -0.75f,        0.0f, 1.0f, 0.0f, 1.0f,
        -0.75f, -0.75f,        0.0f, 1.0f, 0.0f, 1.0f,
        -0.75f, -0.25f,        0.0f, 1.0f, 0.0f, 1.0f,
    };

    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float), 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6*sizeof(float), (const void*)8);

    unsigned int indices[] = {
        0, 1, 2, 3,
        4, 5, 6, 7
    };

    GLuint indexBuffer;
    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(buffer);
        glDrawElements(GL_QUADS, 8, GL_UNSIGNED_INT, NULL);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(programId);
    glfwTerminate();
}
