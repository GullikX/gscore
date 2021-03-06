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

#include <fluidsynth.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlschemas.h>

#include <X11/Xatom.h>

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/* Set version string if not set by makefile */
#ifndef VERSION
#define VERSION "UNKNOWN_VERSION"
#endif


/* Application states */
typedef enum {
    OBJECT_MODE,
    EDIT_MODE,
} State;


/* Type declarations (auto-generated at compile-time) */
#include "typedeclarations.h"


/* Function declarations (auto-generated at compile-time) */
#include "functiondeclarations.h"


/* XSD-schema for file format (auto-generated at compile-time) */
#include "fileformatschema.h"


/* Simple types */
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


/* Configuration */
#include "config.h"


/*Type definitions*/
struct HashMap {
    int nBuckets;
    HashMapEntry** buckets;
};

struct HashMapEntry {
    char* key;
    int value;
    HashMapEntry* next;
};

struct Application {
    const char* filename;
    Score* scoreCurrent;
    Block** blockCurrent;
    Block** blockPrevious;
    EditView* editView;
    ObjectView* objectView;
    Renderer* renderer;
    Synth* synth;
    XEvents* xevents;
    State state;
};

struct Block {
    char name[MAX_BLOCK_NAME_LENGTH];
    Vector4 color;
    MidiMessage* midiMessageRoot;
};

struct GridItem {
    int iRow;
    int iColumn;
    int nRows;
    int nColumns;
    Vector4 color;
    float indicatorValue;
};

struct EditView {
    Score* score;
    int transpose;
    GridItem gridlinesVertical[BLOCK_MEASURES];
    GridItem gridlinesHorizontal[EDIT_MODE_OCTAVES * NOTES_IN_OCTAVE];
    GridItem cursor;
    MidiMessage* midiMessageHeld;
    int playStartTime;
    float playStartPosition;
    bool playRepeat;
    bool ctrlPressed;
    bool ignoreNoteOff;
};

struct MidiMessage {
    int type;
    float time;
    int pitch;
    float velocity;
    MidiMessage* next;
    MidiMessage* prev;
};

struct ObjectView {
    Score* score;
    GridItem gridlinesHorizontal[N_TRACKS_MAX];
    GridItem cursor;
    float viewHeight;
    int playStartTime;
    int iPlayStartBlock;
    bool playRepeat;
    bool ctrlPressed;
    char trackVelocityString[64];
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
    Vector4 clearColor;
};

struct Track {
    char programName[SYNTH_PROGRAM_NAME_LENGTH_MAX];
    float velocity;
    Block** blocks[SCORE_LENGTH_MAX];
    float blockVelocities[SCORE_LENGTH_MAX];
    bool ignoreNoteOff;
};

struct Score {
    int tempo;
    char tempoString[64];
    int nBeatsPerMeasure;
    Block* blocks[MAX_BLOCKS];
    Track* tracks[N_TRACKS_MAX];
    int nBlocks;
    int nTracks;
    int scoreLength;
    char blockListString[MAX_BLOCKS * (MAX_BLOCK_NAME_LENGTH + 1) + 1];
    KeySignature keySignature;
    xmlNode* nodeMetadata;
};

struct Synth {
    fluid_settings_t* settings;
    fluid_synth_t* fluidSynth;
    fluid_audio_driver_t* audioDriver;
    int soundFontIds[MAX_SOUNDFONTS];
    int nSoundFonts;
    fluid_sequencer_t* sequencer;
    fluid_seq_id_t synthSequencerId;
    fluid_seq_id_t callbackId;
    HashMap* instrumentMap;
    char* instrumentListString;
};

struct XEvents {
    Display* x11Display;
    Window x11Window;
    Atom atoms[ATOM_COUNT];
};
