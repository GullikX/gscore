Renderer* Renderer_getInstance() {
    static Renderer* self = NULL;
    if (self) return self;

    self = ecalloc(1, sizeof(*self));

    if (!glfwInit()) {
        die("Failed to initialize GLFW");
    }

    self->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (!self->window) {
        die("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(self->window);
    Input_setupCallbacks(self->window);
    printf("OpenGL %s\n", glGetString(GL_VERSION));

    if (glewInit() != GLEW_OK) {
        die("Failed to initialize GLEW");
    }

    GLuint programId = createProgram();
    glUseProgram(programId);

    glGenBuffers(1, &self->vertexBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, self->vertexBufferId);
    glBufferData(GL_ARRAY_BUFFER, RENDERER_MAX_VERTICES*sizeof(Vertex), NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, color));

    self->nVerticesEnqueued = 0;

    unsigned int indices[RENDERER_MAX_VERTICES];
    for (int i = 0; i < RENDERER_MAX_VERTICES; i++) {
        indices[i] = i;
    }

    GLuint indexBuffer;
    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    return self;
}


void Renderer_stop() {
    Renderer* self = Renderer_getInstance();
    glfwSetWindowShouldClose(self->window, GL_TRUE);
}


int Renderer_running() {
    Renderer* self = Renderer_getInstance();
    return !glfwWindowShouldClose(self->window);
}


void Renderer_updateScreen() {
    Renderer* self = Renderer_getInstance();

    glClearColor(COLOR_BACKGROUND.x, COLOR_BACKGROUND.y, COLOR_BACKGROUND.z, COLOR_BACKGROUND.w);
    glClear(GL_COLOR_BUFFER_BIT);

    {
        Quad quad;
        quad.vertices[0].position.x = 0.25f; quad.vertices[0].position.y = 0.25f;
        quad.vertices[1].position.x = 0.25f; quad.vertices[1].position.y = 0.75f;
        quad.vertices[2].position.x = 0.75f; quad.vertices[2].position.y = 0.75f;
        quad.vertices[3].position.x = 0.75f; quad.vertices[3].position.y = 0.25f;
        for (int i = 0; i < 4; i++) {
            quad.vertices[i].color = COLOR_1;
        }
        Renderer_enqueueDraw(&quad);
    }

    {
        Quad quad;
        quad.vertices[0].position.x = -0.25f; quad.vertices[0].position.y = -0.25f;
        quad.vertices[1].position.x = -0.25f; quad.vertices[1].position.y = -0.75f;
        quad.vertices[2].position.x = -0.75f; quad.vertices[2].position.y = -0.75f;
        quad.vertices[3].position.x = -0.75f; quad.vertices[3].position.y = -0.25f;
        for (int i = 0; i < 4; i++) {
            quad.vertices[i].color = COLOR_2;
        }
        Renderer_enqueueDraw(&quad);
    }

    glBindBuffer(GL_ARRAY_BUFFER, self->vertexBufferId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(self->vertices), self->vertices);

    glBindVertexArray(self->vertexBufferId);
    glDrawElements(GL_QUADS, self->nVerticesEnqueued, GL_UNSIGNED_INT, NULL);
    self->nVerticesEnqueued = 0;
    glfwSwapBuffers(self->window);
    glfwPollEvents();
}


void Renderer_enqueueDraw(Quad* quad) {
    Renderer* self = Renderer_getInstance();
    for (int i = 0; i < 4; i++) {
        if (self->nVerticesEnqueued >= RENDERER_MAX_VERTICES) {
            die("Vertex limit reached");  /* TODO: Handle this better */
        }
        self->vertices[self->nVerticesEnqueued].position = quad->vertices[i].position;
        self->vertices[self->nVerticesEnqueued].color = quad->vertices[i].color;
        self->nVerticesEnqueued++;
    }
}

void Renderer_updateViewportSize(int width, int height) {
    glViewport(0, 0, width, height);
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
