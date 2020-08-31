/* Copyright (C) 2020 Martin Gulliksson <martin@gullik.cc>
 *
 * This file is part of GScore.
 *
 * GScore is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, version 3.
 *
 * GScore is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>
 *
 */

#include <fluidsynth.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <X11/Xatom.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/* Type declarations */
typedef struct Vector2 Vector2;
typedef struct Vector4 Vector4;
typedef struct Vertex Vertex;
typedef struct MidiMessage MidiMessage;
typedef struct Application Application;
typedef struct Block Block;
typedef struct GridItem GridItem;
typedef struct Synth Synth;
typedef struct BlockPlayer BlockPlayer;
typedef struct Renderer Renderer;
typedef struct Score Score;
typedef struct ScorePlayer ScorePlayer;
typedef struct Track Track;
typedef struct EditView EditView;
typedef struct ObjectView ObjectView;
typedef struct XEvents XEvents;


/* Type definitions */
typedef enum {
    OBJECT_MODE,
    EDIT_MODE,
} State;

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

struct MidiMessage {
    int type;
    float time;
    int pitch;
    int velocity;
    MidiMessage* next;
    MidiMessage* prev;
};

struct Application {
    const char* filename;
    Score* scoreCurrent;
    State state;
};

struct Block {
    const char* name;
    Vector4 color;
    MidiMessage* midiMessageRoot;
};

struct GridItem {
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
    char* instrumentListString;
};

struct BlockPlayer {
    int channel;
    MidiMessage* midiMessage;
    bool playing;
    bool repeat;
    float startTime;
    float startPosition;
    int tempoBpm;
    char tempoBpmString[64];
};


/* Function declarations needed for config */
char* BlockPlayer_getTempoBpmString(void);
char* Synth_getInstrumentListString(void);


/* Configuration */
#include "config.h"


/* Type definitions dependent on configuration */
struct EditView {
    GridItem gridlinesVertical[BLOCK_MEASURES];
    GridItem gridlinesHorizontal[OCTAVES];
    GridItem cursor;
    Block* blockCurrent;
    MidiMessage* midiMessageHeld;
};

struct ObjectView {
    GridItem gridlinesHorizontal[N_TRACKS];
    GridItem cursor;
    float viewHeight;
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

struct ScorePlayer {
    Score* score;
    bool playing;
    float startTime;
    int iBlock;
};

struct Track{
    int program;
    int velocity;
    Block* blocks[SCORE_LENGTH];
};

struct Score {
    int tempo;
    Block blocks[MAX_BLOCKS];
    Track tracks[N_TRACKS];
};

struct XEvents {
    Display* x11Display;
    Window x11Window;
    Atom atoms[ATOM_COUNT];
};


/* Function declarations */
/* application.c */
Application* Application_new(const char* const filename);
Application* Application_free(Application* self);
Application* Application_getInstance(void);
void Application_run(Application* self);
State Application_getState(Application* self);
void Application_switchState(Application* self);

/* editview.c */
EditView* EditView_getInstance(void);
void EditView_previewNote(void);
void EditView_addNote(void);
void EditView_dragNote(void);
void EditView_removeNote(void);
void EditView_draw(void);
void EditView_drawItem(GridItem* item, float offset);
bool EditView_updateCursorPosition(float x, float y);
int EditView_rowIndexToNoteKey(int iRow);
int EditView_pitchToRowIndex(int pitch);
MidiMessage* EditView_addMidiMessage(int type, float time, int pitch, int velocity);
void EditView_removeMidiMessage(MidiMessage* midiMessage);
int EditView_xCoordToColumnIndex(float x);
int EditView_yCoordToRowIndex(float y);

/* filereader.c */
Score* FileReader_read(const char* const filename);
Score* FileReader_createScoreFromFile(const char* const filename);
void FileReader_createBlockDefs(Score* score, xmlNode* nodeBlockDefs);
void FileReader_createTracks(Score* score, xmlNode* nodeTracks);
Score* FileReader_createNewEmptyScore(void);

/* filewriter.c */
void FileWriter_write(const Score* const score, const char* const filename);
void FileWriter_writeBlockDefs(const Score* const score, xmlNode* nodeRoot);
void FileWriter_writeTracks(const Score* const score, xmlNode* nodeRoot);

/* input.c */
void Input_setupCallbacks(GLFWwindow* window);
void Input_keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void Input_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void Input_cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void Input_scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void Input_windowSizeCallback(GLFWwindow* window, int width, int height);
void Input_keyCallbackObjectMode(GLFWwindow* window, int key, int scancode, int action, int mods);
void Input_mouseButtonCallbackObjectMode(GLFWwindow* window, int button, int action, int mods);
void Input_cursorPosCallbackObjectMode(GLFWwindow* window, double x, double y);
void Input_keyCallbackEditMode(GLFWwindow* window, int key, int scancode, int action, int mods);
void Input_mouseButtonCallbackEditMode(GLFWwindow* window, int button, int action, int mods);
void Input_cursorPosCallbackEditMode(GLFWwindow* window, double x, double y);

/* main.c */
int main(int argc, char* argv[]);

/* objectview.c */
ObjectView* ObjectView_getInstance(void);
void ObjectView_addBlock(void);
void ObjectView_removeBlock(void);
void ObjectView_draw(void);
void ObjectView_drawItem(GridItem* item, float offset);
bool ObjectView_updateCursorPosition(float x, float y);
int ObjectView_xCoordToColumnIndex(float x);
int ObjectView_yCoordToRowIndex(float y);

/* player.c */
BlockPlayer* BlockPlayer_getInstance(void);
void BlockPlayer_setTempoBpm(int tempoBpm);
bool BlockPlayer_playing(void);
void BlockPlayer_playBlock(Block* block, float startPosition, bool repeat);
void BlockPlayer_stop(void);
void BlockPlayer_update(void);
void BlockPlayer_drawCursor(void);

/* renderer.c */
Renderer* Renderer_getInstance(void);
void Renderer_stop(void);
int Renderer_running(void);
void Renderer_updateScreen(void);
void Renderer_drawQuad(float x1, float x2, float y1, float y2, Vector4 color);
void Renderer_updateViewportSize(int width, int height);
GLuint createShader(const GLenum type, const char* const shaderSource);
GLuint createProgram(void);

/* scoreplayer.c */
ScorePlayer* ScorePlayer_getInstance(void);
bool ScorePlayer_playing(void);
void ScorePlayer_playScore(Score* score);
void ScorePlayer_stop(void);
void ScorePlayer_update(void);
void ScorePlayer_drawCursor(void);

/* synth.c */
Synth* Synth_getInstance(void);
void Synth_setProgramById(int channel, int programId);
void Synth_setProgramByName(int channel, const char* const instrumentName);
void Synth_processMessage(int channel, MidiMessage* midiMessage);
void Synth_noteOn(int key);
void Synth_noteOff(int key);
void Synth_noteOffAll(void);

/* util.c */
void die(const char* const format, ...);
void* ecalloc(size_t nItems, size_t itemSize);
bool fileExists(const char* const filename);
void spawnSetXProp(int atomId);
void spawn(const char* const cmd, const char* const pipeData);

/* xevents.c */
XEvents* XEvents_getInstance(void);
void XEvents_processXEvents(void);
