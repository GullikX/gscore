/* Copyright (C) 2020-2024 Martin Gulliksson <martin@gullik.cc>
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

#include "renderer.h"

#include "common/structs/color.h"
#include "common/structs/quad.h"
#include "common/structs/vector2.h"
#include "common/structs/vector2i.h"
#include "common/util/alloc.h"
#include "common/util/log.h"
#include "config/config.h"
#include "events/events.h"

#include <GL/glew.h>


enum {
    VERTICES_PER_QUAD = 4,
    MAX_QUADS_PER_DRAW_CALL = 1024,
    MAX_VERTICES_PER_DRAW_CALL = VERTICES_PER_QUAD * MAX_QUADS_PER_DRAW_CALL,
};


typedef struct {
    Vector2 position;
    Color color;
} Vertex;


struct Renderer {
    GLuint vertexBufferId;
    Quad quadsPending[MAX_QUADS_PER_DRAW_CALL];
    int nQuadsPending;
};


static const char* const VERTEX_SHADER_SOURCE =
    "#version 120\n"
    "attribute vec4 color;"
    "void main() {"
    "   gl_FrontColor = color;"
    "   gl_Position = gl_Vertex;"
    "}";

static const char* const FRAGMENT_SHADER_SOURCE =
    "#version 120\n"
    "void main() {"
    "    gl_FragColor = gl_Color;"
    "}";


static void Renderer_onRequestDrawQuad(Renderer* self, void* sender, Quad* quad);
static void Renderer_onProcessFrameBegin(Renderer* self, void* sender, float* timeDelta);
static void Renderer_onProcessFrameEnd(Renderer* self, void* sender, float* timeDelta);
static void Renderer_onViewportSizeUpdated(Renderer* self, void* sender, Vector2i* size);
static void Renderer_flushQuadBuffer(Renderer* self);
static Vertex createQuadVertex(Quad quad, int id);
static Vector2 screenSpaceToOpenglCoords(Vector2 position);
static GLuint createShaderProgram(void);
static GLuint createShader(const GLenum type, const char* const shaderSource);


Renderer* Renderer_new(void) {
    Renderer* self = ecalloc(1, sizeof(*self));
    Log_info("OpenGL version: %s", glGetString(GL_VERSION));
    Log_info("OpenGL renderer: %s", glGetString(GL_RENDERER));
    if (glewInit() != GLEW_OK) {
        Log_fatal("Failed to initialize GLEW");
    }

    glUseProgram(createShaderProgram());

    glGenBuffers(1, &self->vertexBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, self->vertexBufferId);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES_PER_DRAW_CALL*sizeof(Vertex), NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, color));

    unsigned int indices[MAX_VERTICES_PER_DRAW_CALL];
    for (int i = 0; i < MAX_VERTICES_PER_DRAW_CALL; i++) {
        indices[i] = i;
    }

    GLuint indexBuffer;
    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    Event_subscribe(EVENT_REQUEST_DRAW_QUAD, self, EVENT_CALLBACK(Renderer_onRequestDrawQuad), sizeof(Quad));
    Event_subscribe(EVENT_PROCESS_FRAME_BEGIN, self, EVENT_CALLBACK(Renderer_onProcessFrameBegin), sizeof(float));
    Event_subscribe(EVENT_PROCESS_FRAME_END, self, EVENT_CALLBACK(Renderer_onProcessFrameEnd), sizeof(float));
    Event_subscribe(EVENT_VIEWPORT_SIZE_UPDATED, self, EVENT_CALLBACK(Renderer_onViewportSizeUpdated), sizeof(Vector2i));

    return self;
}


void Renderer_free(Renderer** pself) {
    Renderer* self = *pself;

    Event_unsubscribe(EVENT_REQUEST_DRAW_QUAD, self, EVENT_CALLBACK(Renderer_onRequestDrawQuad), sizeof(Quad));
    Event_unsubscribe(EVENT_PROCESS_FRAME_BEGIN, self, EVENT_CALLBACK(Renderer_onProcessFrameBegin), sizeof(float));
    Event_unsubscribe(EVENT_PROCESS_FRAME_END, self, EVENT_CALLBACK(Renderer_onProcessFrameEnd), sizeof(float));
    Event_unsubscribe(EVENT_VIEWPORT_SIZE_UPDATED, self, EVENT_CALLBACK(Renderer_onViewportSizeUpdated), sizeof(Vector2i));

    sfree((void**)pself);
}


static void Renderer_onRequestDrawQuad(Renderer* self, void* sender, Quad* quad) {
    (void)sender;
    if (self->nQuadsPending == MAX_QUADS_PER_DRAW_CALL) {
        Renderer_flushQuadBuffer(self);
        Log_assert(self->nQuadsPending == 0, "Quads still pending after flushing buffer");
    }

    self->quadsPending[self->nQuadsPending] = *quad;
    self->nQuadsPending++;
}


static void Renderer_onProcessFrameBegin(Renderer* self, void* sender, float* timeDelta) {
    (void)sender; (void)timeDelta;
    glClearColor(BACKGROUND_COLOR.r, BACKGROUND_COLOR.g, BACKGROUND_COLOR.b, BACKGROUND_COLOR.a);
    glClear(GL_COLOR_BUFFER_BIT);
    Log_assert(self->nQuadsPending == 0, "Quads already pending in beginning of process frame");
}


static void Renderer_onProcessFrameEnd(Renderer* self, void* sender, float* timeDelta) {
    (void)sender; (void)timeDelta;
    Renderer_flushQuadBuffer(self);
}


static void Renderer_onViewportSizeUpdated(Renderer* self, void* sender, Vector2i* size) {
    (void)self;
    (void)sender;
    glViewport(0, 0, size->x, size->y);
}


static void Renderer_flushQuadBuffer(Renderer* self) {
    if (self->nQuadsPending > 0) {
        Vertex vertices[MAX_VERTICES_PER_DRAW_CALL];
        for (int iQuad = 0; iQuad < self->nQuadsPending; iQuad++) {
            for (int iVertex = 0; iVertex < VERTICES_PER_QUAD; iVertex++) {
                vertices[iQuad * VERTICES_PER_QUAD + iVertex] = createQuadVertex(self->quadsPending[iQuad], iVertex);
            }
        }

        GLsizei nVertices = VERTICES_PER_QUAD * self->nQuadsPending;

        glBindBuffer(GL_ARRAY_BUFFER, self->vertexBufferId);
        glBufferSubData(GL_ARRAY_BUFFER, 0, nVertices * sizeof(Vertex), vertices);

        glBindVertexArray(self->vertexBufferId);
        glDrawElements(GL_QUADS, nVertices, GL_UNSIGNED_INT, NULL);

        self->nQuadsPending = 0;
    }
}


static Vertex createQuadVertex(Quad quad, int id) {
    Log_assert(id >= 0 && id < VERTICES_PER_QUAD, "Invalid vertex id: %d", id);
    Vector2 position;
    switch (id) {
        case 0:
            position.x = quad.position.x;
            position.y = quad.position.y;
            break;
        case 1:
            position.x = quad.position.x;
            position.y = quad.position.y + quad.size.y;
            break;
        case 2:
            position.x = quad.position.x + quad.size.x;
            position.y = quad.position.y + quad.size.y;
            break;
        case 3:
            position.x = quad.position.x + quad.size.x;
            position.y = quad.position.y;
            break;
    }
    return (Vertex){screenSpaceToOpenglCoords(position), quad.color};
}


static Vector2 screenSpaceToOpenglCoords(Vector2 position) {
    return (Vector2){2.0 * position.x - 1.0f, -(2.0f * position.y - 1.0f)};
}


static GLuint createShaderProgram(void) {
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


static GLuint createShader(const GLenum type, const char* const shaderSource) {
    GLuint shaderId = glCreateShader(type);
    glShaderSource(shaderId, 1, &shaderSource, NULL);
    glCompileShader(shaderId);

    int compilationResult = 0;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compilationResult);
    Log_assert(compilationResult == GL_TRUE, "Failed to compile shader");

    return shaderId;
}
