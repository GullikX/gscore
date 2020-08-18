#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


/* Type declarations */
typedef struct Vector2 Vector2;
typedef struct Vector4 Vector4;
typedef struct Vertex Vertex;
typedef struct Quad Quad;
typedef struct Note Note;
typedef struct Gridline Gridline;
typedef struct Renderer Renderer;
typedef struct Canvas Canvas;


/* Type definitions */
struct Vector2 {
    float x, y;
};

struct Vector4 {
    float x, y, z, w;
};

struct Vertex {
    Vector2 position;
    Vector4 color;
};

struct Quad {
    Vertex vertices[4];
};

struct Note {
    int start;
    int end;
    int pitch;
    int velocity;
};

struct Gridline {
    float x1, x2, y1, y2;
};


/* Configuration */
#include "config.h"


/* Type definitions dependent on configuration */
struct Canvas {
    Note* notes[CANVAS_MAX_NOTES];
    Gridline gridlinesVertical[BLOCK_MEASURES];
    Gridline gridlinesHorizontal[OCTAVES];
    int noteIndex;
};

struct Renderer {
     GLFWwindow* window;
     GLuint programId;
     GLuint vertexBufferId;
     Vertex* vertexBufferTarget;
     Vertex vertices[RENDERER_MAX_VERTICES];
     int nVerticesEnqueued;
};


/* Function declarations */
/* canvas.c */
Canvas* Canvas_getInstance();
void Canvas_addNote(Note* note);
void Canvas_draw();

/* input.c */
void Input_setupCallbacks(GLFWwindow* window);
void Input_keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void Input_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void Input_scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void Input_windowSizeCallback(GLFWwindow* window, int width, int height);

/* main.c */
int main();

/* note.c */
Note* Note_new(int start, int end, int pitch, int velocity);
void Note_draw();

/* renderer.c */
Renderer* Renderer_getInstance();
void Renderer_stop();
int Renderer_running();
void Renderer_updateScreen();
void Renderer_enqueueDraw(Quad* quad);
void Renderer_updateViewportSize(int width, int height);
GLuint createShader(const GLenum type, const char* const shaderSource);
GLuint createProgram();

/* util.c */
void die(const char* const message);
void* ecalloc(size_t nItems, size_t itemSize);
