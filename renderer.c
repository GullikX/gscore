Renderer* Renderer_new() {
    Renderer* self = ecalloc(1, sizeof(*self));

    if (!glfwInit()) {
        die("Failed to initialize GLFW");
    }

    self->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (!self->window) {
        die("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(self->window);
    glfwSetWindowSizeCallback(self->window, windowSizeCallback);
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

    glGenBuffers(1, &self->vertexBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, self->vertexBufferId);
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

    return self;
}


Renderer* Renderer_free(Renderer* self) {
    if (!self) return NULL;
    glDeleteProgram(self->programId);
    glfwDestroyWindow(self->window);
    glfwTerminate();
    free(self);
    return NULL;
}


int Renderer_running(Renderer* self) {
    return !glfwWindowShouldClose(self->window);
}


void Renderer_update(Renderer* self) {
    if (glfwGetKey(self->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(self->window, GL_TRUE);
    }
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(self->vertexBufferId);
    glDrawElements(GL_QUADS, 8, GL_UNSIGNED_INT, NULL);
    glfwSwapBuffers(self->window);
    glfwPollEvents();
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

    GLuint vertexShaderId = createShader(GL_VERTEX_SHADER, VERTEX_SHADER_SOURCE);
    GLuint fragmentShaderId = createShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_SOURCE);

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
