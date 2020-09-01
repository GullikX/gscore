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
    float velocity;
    MidiMessage* next;
    MidiMessage* prev;
};

struct Application {
    const char* filename;
    Score* scoreCurrent;
    Block* blockCurrent;
    EditView* editView;
    ObjectView* objectView;
    Renderer* renderer;
    Synth* synth;
    XEvents* xevents;
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
    fluid_sequencer_t* sequencer;
    int synthSequencerId;
    char* instrumentListString;
};


/* Function declarations (auto-generated at compile-time) */
#include "functiondeclarations.h"


/* Configuration */
#include "config.h"


/* Type definitions dependent on configuration */
struct EditView {
    GridItem gridlinesVertical[BLOCK_MEASURES];
    GridItem gridlinesHorizontal[OCTAVES];
    GridItem cursor;
    MidiMessage* midiMessageHeld;
    int playStartTime;
    int tempo;
    char tempoString[64];
};

struct ObjectView {
    GridItem gridlinesHorizontal[N_TRACKS];
    GridItem cursor;
    float viewHeight;
    ScorePlayer* player;
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
    MidiMessage* midiMessages[N_TRACKS];
};

struct Track{
    int program;
    float velocity;
    Block* blocks[SCORE_LENGTH];
    float blockVelocities[SCORE_LENGTH];
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
