#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <fluidsynth.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xatom.h>


/* Type declarations */
typedef struct Vector2 Vector2;
typedef struct Vector4 Vector4;
typedef struct Vertex Vertex;
typedef struct Quad Quad;
typedef struct CanvasItem CanvasItem;
typedef struct Synth Synth;
typedef struct Player Player;
typedef struct Renderer Renderer;
typedef struct Canvas Canvas;
typedef struct XEvents XEvents;


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

struct CanvasItem {
    int iRow;
    int iColumn;
    int nRows;
    int nColumns;
    Vector4 color;
};

struct Synth {
    fluid_settings_t* settings;
    fluid_synth_t* fluidSynth;
    fluid_audio_driver_t* audioDriver;
};

struct Player {
    bool playing;
    double startTime;
    int tempoBpm;
};


/* Configuration */
#include "config.h"


/* Type definitions dependent on configuration */
struct Canvas {
    CanvasItem gridlinesVertical[BLOCK_MEASURES];
    CanvasItem gridlinesHorizontal[OCTAVES];
    CanvasItem playerCursor;
    CanvasItem notes[CANVAS_MAX_NOTES];
    CanvasItem cursor;
    int noteIndex;
};

struct Renderer {
     GLFWwindow* window;
     GLuint programId;
     GLuint vertexBufferId;
     Vertex* vertexBufferTarget;
     Vertex vertices[RENDERER_MAX_VERTICES];
     int nVerticesEnqueued;
     int viewportWidth;
     int viewportHeight;
};

struct XEvents {
    Display* x11Display;
    Window x11Window;
    Atom atoms[ATOM_COUNT];
};


/* Function declarations */
/* canvas.c */
Canvas* Canvas_getInstance();
void Canvas_previewNote();
void Canvas_addNote();
void Canvas_removeNote();
void Canvas_draw();
void Canvas_drawItem(CanvasItem* canvasItem, float offset);
bool Canvas_updateCursorPosition(float x, float y);
void Canvas_updatePlayerCursorPosition(float x);
void Canvas_resetPlayerCursorPosition();
int Canvas_rowIndexToNoteKey(int iRow);

/* input.c */
void Input_setupCallbacks(GLFWwindow* window);
void Input_keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void Input_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void Input_cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void Input_scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void Input_windowSizeCallback(GLFWwindow* window, int width, int height);

/* main.c */
int main();

/* player.c */
Player* Player_getInstance();
void Player_setTempoBpm(int tempoBpm);
void Player_toggle();
bool Player_playing();
void Player_start();
void Player_stop();
void Player_update();

/* renderer.c */
Renderer* Renderer_getInstance();
void Renderer_stop();
int Renderer_running();
void Renderer_updateScreen();
void Renderer_enqueueDraw(Quad* quad);
void Renderer_updateViewportSize(int width, int height);
int Renderer_xCoordToColumnIndex(int x);
int Renderer_yCoordToRowIndex(int y);
GLuint createShader(const GLenum type, const char* const shaderSource);
GLuint createProgram();

/* synth.c */
Synth* Synth_getInstance();
void Synth_setProgram(int channelId, int programId);
void Synth_noteOn(int key);
void Synth_noteOffAll();

/* util.c */
void die(const char* const message);
void* ecalloc(size_t nItems, size_t itemSize);

/* xevents.c */
XEvents* XEvents_getInstance();
void XEvents_processXEvents();
