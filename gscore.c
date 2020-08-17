#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>


const char* const vertexShaderSource =
    "#version 120\n"
    "void main()\n"
    "{\n"
    "   gl_Position = gl_Vertex;\n"
    "}\0";


const char* const fragmentShaderSource =
    "#version 120\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = vec4(0.2f, 0.5f, 1.0f, 1.0f);\n"
    "}\0";


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

    GLuint programId = createProgram();
    glUseProgram(programId);

    float positions[6] = {
        -0.5f, -0.5f,
         0.0f,  0.5f,
         0.5f, -0.5f,
    };

    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, 6*sizeof(float), positions, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), 0);

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GL_TRUE);
        }
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(programId);
    glfwTerminate();
}
