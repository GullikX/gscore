#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>


/* Type declarations */
typedef struct Renderer Renderer;


/* Configuration */
#include "config.h"


/* Type definitions */
struct Renderer {
     GLFWwindow* window;
     GLuint programId;
     GLuint vertexBufferId;
};


/* Function declarations */
/* main.c */
int main();

/* renderer.c */
Renderer* Renderer_new();
Renderer* Renderer_free(Renderer* self);
int Renderer_running(Renderer* self);
void Renderer_update(Renderer* self);
GLuint createShader(const GLenum type, const char* const shaderSource);
GLuint createProgram();
void windowSizeCallback(GLFWwindow* window, int width, int height);

/* util.c */
void die(const char* const message);
void* ecalloc(size_t nItems, size_t itemSize);
