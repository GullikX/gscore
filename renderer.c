/* Copyright (C) 2020-2021 Martin Gulliksson <martin@gullik.cc>
 *
 * This file is part of gscore.
 *
 * gscore is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 3.
 *
 * gscore is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>
 *
 */

static Renderer* Renderer_new(void) {
    Renderer* self = ecalloc(1, sizeof(*self));

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

    if (MOUSE_POINTER_HIDDEN) {
        glfwSetInputMode(self->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }

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


static Renderer* Renderer_free(Renderer* self) {
    glfwDestroyWindow(self->window);
    glfwTerminate();
    free(self);
    return NULL;
}


static void Renderer_stop(Renderer* self) {
    glfwSetWindowShouldClose(self->window, GL_TRUE);
}


static int Renderer_running(Renderer* self) {
    return !glfwWindowShouldClose(self->window);
}


static void Renderer_updateScreen(Renderer* self) {
    Renderer_flushVertexBuffer(self);
    glfwSwapBuffers(self->window);
    glClearColor(COLOR_BACKGROUND.x, COLOR_BACKGROUND.y, COLOR_BACKGROUND.z, COLOR_BACKGROUND.w);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwPollEvents();
}


static void Renderer_flushVertexBuffer(Renderer* self) {
    glBindBuffer(GL_ARRAY_BUFFER, self->vertexBufferId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(self->vertices), self->vertices);

    glBindVertexArray(self->vertexBufferId);
    glDrawElements(GL_QUADS, self->nVerticesEnqueued, GL_UNSIGNED_INT, NULL);
    self->nVerticesEnqueued = 0;
}


static void Renderer_drawQuad(Renderer* self, float x1, float x2, float y1, float y2, Vector4 color) {
    if (self->nVerticesEnqueued + 4 >= RENDERER_MAX_VERTICES) {
        Renderer_flushVertexBuffer(self);
    }

    Vector2 positions[4];
    positions[0].x = x1; positions[0].y = y1;
    positions[1].x = x1; positions[1].y = y2;
    positions[2].x = x2; positions[2].y = y2;
    positions[3].x = x2; positions[3].y = y1;

    for (int i = 0; i < 4; i++) {
        self->vertices[self->nVerticesEnqueued].position = positions[i];
        self->vertices[self->nVerticesEnqueued].color = color;
        self->nVerticesEnqueued++;
    }
}


static void Renderer_updateViewportSize(Renderer* self, int width, int height) {
    self->viewportWidth = width;
    self->viewportHeight = height;
    glViewport(0, 0, width, height);
}


static GLuint createShader(const GLenum type, const char* const shaderSource) {
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
        die("Failed to compile %s shader! Message:\n    %s", typeString, message);
        free(message);
    }
    return shaderId;
}


static GLuint createProgram(void) {
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
