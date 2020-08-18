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
typedef struct Renderer Renderer;


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


/* Configuration */
#include "config.h"


/* Type definitions dependent on configuration */
struct Renderer {
     GLFWwindow* window;
     GLuint programId;
     GLuint vertexBufferId;
     Vertex* vertexBufferTarget;
     Vertex vertices[RENDERER_MAX_VERTICES];
     int nVerticesEnqueued;
};


/* Function declarations */
/* input.c */
void Input_setupCallbacks(GLFWwindow* window);
void Input_keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void Input_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void Input_scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void Input_windowSizeCallback(GLFWwindow* window, int width, int height);

/* main.c */
int main();

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
