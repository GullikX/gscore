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

#include "editview.h"

#include "common/constants/applicationstate.h"
#include "common/constants/fluidmidi.h"
#include "common/constants/input.h"
#include "common/constants/keysignatures.h"
#include "common/score/score.h"
#include "common/structs/charevent.h"
#include "common/structs/color.h"
#include "common/structs/keyevent.h"
#include "common/structs/midimessage.h"
#include "common/structs/mousebuttonevent.h"
#include "common/structs/note.h"
#include "common/structs/sequencerrequest.h"
#include "common/structs/vector2i.h"
#include "common/util/alloc.h"
#include "common/util/colors.h"
#include "common/util/math.h"
#include "common/util/inputmatcher.h"
#include "common/util/hashmap.h"
#include "common/util/log.h"
#include "common/visual/grid/grid.h"
#include "config/config.h"
#include "events/events.h"

#include <stdbool.h>

enum {
    N_NOTES_TO_SHOW_DEFAULT = 5 * N_NOTES_IN_OCTAVE,
    N_NOTES_TO_SHOW_MIN = N_NOTES_IN_OCTAVE,
    N_NOTES_TO_SHOW_MAX = MIDI_MESSAGE_PITCH_COUNT,
    N_BEATS_PER_MEASURE = N_BEATS_PER_MEASURE_DEFAULT,
    N_BEATS_TOTAL = N_BEATS_PER_MEASURE * N_BLOCK_MEASURES,
    MEASURE_RESOLUTION = 4,
    NOTE_PREVIEW_MIDI_CHANNEL = 0,
};

static const Vector2i CURSOR_SIZE = {1, 1};

static const char* const NOTE_ACTION_ADD = "Adding";
static const char* const NOTE_ACTION_REMOVE = "Removing";
static const char* const NOTE_ACTION_PREVIEW = "Previewing";


typedef enum {
    STATE_INVALID,
    STATE_IDLE,
    STATE_DRAGGING,
    STATE_PLAYING,
    STATE_INITIALIZING,
    STATE_INITIALIZING_GHOST,
} State;


struct EditView {
    Score* score;
    State state;
    ApplicationState applicationState;
    Grid* gridlinesGrid;
    Grid* notesGrid;
    Grid* ghostNotesGrid;
    Grid* cursorGrid;
    Grid* dragNoteGrid;
    Color blockColor;
    QuadHandle verticalGridLineHandles[N_BLOCK_MEASURES];
    QuadHandle horizontalGridLineHandles[MIDI_MESSAGE_PITCH_COUNT];
    QuadHandle cursorHandle;
    QuadHandle playbackCursorHandle;
    QuadHandle dragNoteHandle;
    Vector2 mousePosition;
    Vector2i cursorPosition;
    Vector2i cursorPositionPrev;
    Vector2i cursorPositionDragStart;
    Vector2i playbackCursorPosition;
    Vector2i playbackCursorPositionPrev;
    HashMap* noteQuadMap;
    HashMap* spatialQuadMap;
    bool isNotePreviewButtonPressed;
    bool isNoteRemovalButtonPressed;
    bool isZoomKeyPressed;
    bool isPlayingLoop;
    int scrollY;
    int nNotesToShow;
    Vector2i zoomTargetCursorPosition;
    bool ignoreNoteOff;
};


static void EditView_onCharEvent(EditView* self, void* sender, CharEvent* event);
static void EditView_onKeyEvent(EditView* self, void* sender, KeyEvent* event);
static void EditView_onMouseButtonEvent(EditView* self, void* sender, MouseButtonEvent* event);
static void EditView_onMouseMotionEvent(EditView* self, void* sender, Vector2* position);
static void EditView_onMouseScrollwheelEvent(EditView* self, void* sender, Vector2i* offset);
static void EditView_onNoteAdded(EditView* self, void* sender, Note* note);
static void EditView_onNoteRemoved(EditView* self, void* sender, Note* note);
static void EditView_onActiveBlockChanged(EditView* self, void* sender, Color* color);
static void EditView_onBlockColorChanged(EditView* self, void* sender, Color* color);
static void EditView_onKeySignatureChanged(EditView* self, void* sender, int* iKeySignature);
static void EditView_onRequestIgnoreNoteoff(EditView* self, void* sender, int* ignoreNoteOff);
static void EditView_onSequencerStarted(EditView* self, void* sender, SequencerRequest* sequencerRequest);
static void EditView_onSequencerStopped(EditView* self, void* sender, void* unused);
static void EditView_onSequencerProgress(EditView* self, void* sender, float* progress);
static void EditView_onApplicationStateChanged(EditView* self, void* sender, ApplicationState* applicationState);
static void EditView_onCursorPositionUpdated(EditView* self);
static void EditView_addNoteAtCursor(EditView* self);
static void EditView_removeNoteAtCursor(EditView* self);
static void EditView_removeNoteAtPosition(EditView* self, Vector2i position);
static void EditView_previewNoteAtCursor(EditView* self);
static void EditView_stopPreviewingAllNotes(EditView* self);
static void EditView_logNoteAction(EditView* self, const char* const action);
static void EditView_refreshNoteGrid(EditView* self);
static void EditView_hide(EditView* self);
static void EditView_hideGrids(EditView* self);
static void EditView_unhide(EditView* self);
static void EditView_unhideGrids(EditView* self);
static int transformRowIndexToNotePitch(int iRow);
static int transformNotePitchToRowIndex(int pitch);
static Vector2i getGridSize(void);


EditView* EditView_new(Score* score) {
    EditView* self = ecalloc(1, sizeof(*self));

    self->score = score;
    self->nNotesToShow = N_NOTES_TO_SHOW_DEFAULT;
    self->scrollY = (MIDI_MESSAGE_VELOCITY_COUNT - self->nNotesToShow) / 2;

    self->gridlinesGrid = Grid_new(getGridSize(), (Vector2){GRIDLINES_GRID_CELL_SPACING, GRIDLINES_GRID_CELL_SPACING});
    self->notesGrid = Grid_new(getGridSize(), (Vector2){NOTES_GRID_CELL_SPACING, NOTES_GRID_CELL_SPACING});
    self->ghostNotesGrid = Grid_new(getGridSize(), (Vector2){NOTES_GRID_CELL_SPACING, NOTES_GRID_CELL_SPACING});
    self->cursorGrid = Grid_new(getGridSize(), (Vector2){CURSOR_GRID_CELL_SPACING, CURSOR_GRID_CELL_SPACING});
    self->dragNoteGrid = Grid_new(getGridSize(), (Vector2){NOTES_GRID_CELL_SPACING, NOTES_GRID_CELL_SPACING});

    Grid_zoomTo(self->gridlinesGrid, (Vector2i){0, self->scrollY}, (Vector2i){getGridSize().x, self->nNotesToShow});
    Grid_zoomTo(self->notesGrid, (Vector2i){0, self->scrollY}, (Vector2i){getGridSize().x, self->nNotesToShow});
    Grid_zoomTo(self->ghostNotesGrid, (Vector2i){0, self->scrollY}, (Vector2i){getGridSize().x, self->nNotesToShow});
    Grid_zoomTo(self->cursorGrid, (Vector2i){0, self->scrollY}, (Vector2i){getGridSize().x, self->nNotesToShow});
    Grid_zoomTo(self->dragNoteGrid, (Vector2i){0, self->scrollY}, (Vector2i){getGridSize().x, self->nNotesToShow});

    for (int i = 0; i < MIDI_MESSAGE_PITCH_COUNT; i++) {
        Vector2i position = {0, i};
        Vector2i size = {N_BEATS_TOTAL * MEASURE_RESOLUTION, 1};
        KeySignature keySignature = KEY_SIGNATURE_DEFAULT;
        int note = Math_posmod(MIDI_MESSAGE_PITCH_COUNT - 1 - i, N_NOTES_IN_OCTAVE);
        Color color = KEY_SIGNATURES[keySignature][note] ? GRID_LINE_COLOR : BACKGROUND_COLOR;
        self->horizontalGridLineHandles[i] = Grid_addQuad(self->gridlinesGrid, position, size, color);
    }

    for (int i = 0; i < N_BLOCK_MEASURES; i++) {
        Vector2i position = {i * N_BEATS_PER_MEASURE * MEASURE_RESOLUTION, 0};
        Vector2i size = {1, MIDI_MESSAGE_PITCH_COUNT};
        self->verticalGridLineHandles[i] = Grid_addQuad(self->gridlinesGrid, position, size, GRID_LINE_COLOR);
    }

    self->cursorHandle = Grid_addQuad(self->cursorGrid, self->cursorPosition, CURSOR_SIZE, CURSOR_COLOR);

    self->playbackCursorHandle = Grid_addQuad(self->cursorGrid, self->cursorPosition, (Vector2i){1, MIDI_MESSAGE_PITCH_COUNT}, CURSOR_COLOR);
    Grid_hideQuad(self->cursorGrid, self->playbackCursorHandle);

    self->dragNoteHandle = Grid_addQuad(self->dragNoteGrid, self->cursorPosition, (Vector2i){1, 1}, CURSOR_COLOR);
    Grid_hideQuad(self->dragNoteGrid, self->dragNoteHandle);

    self->noteQuadMap = HashMap_new(sizeof(Note));

    self->spatialQuadMap = HashMap_new(sizeof(Vector2i));
    for (int x = 0; x < getGridSize().x; x++) {
        for (int y = 0; y < getGridSize().y; y++) {
            Vector2i position = {x, y};
            HashMap_addItem(self->spatialQuadMap, &position, NULL);
        }
    }

    Event_subscribe(EVENT_CHAR_INPUT, self, EVENT_CALLBACK(EditView_onCharEvent), sizeof(CharEvent));
    Event_subscribe(EVENT_KEY_INPUT, self, EVENT_CALLBACK(EditView_onKeyEvent), sizeof(KeyEvent));
    Event_subscribe(EVENT_MOUSE_BUTTON_INPUT, self, EVENT_CALLBACK(EditView_onMouseButtonEvent), sizeof(MouseButtonEvent));
    Event_subscribe(EVENT_MOUSE_MOTION_INPUT, self, EVENT_CALLBACK(EditView_onMouseMotionEvent), sizeof(Vector2));
    Event_subscribe(EVENT_MOUSE_SCROLLWHEEL_INPUT, self, EVENT_CALLBACK(EditView_onMouseScrollwheelEvent), sizeof(Vector2i));
    Event_subscribe(EVENT_SEQUENCER_STARTED, self, EVENT_CALLBACK(EditView_onSequencerStarted), sizeof(SequencerRequest));
    Event_subscribe(EVENT_SEQUENCER_STOPPED, self, EVENT_CALLBACK(EditView_onSequencerStopped), 0);
    Event_subscribe(EVENT_SEQUENCER_PROGRESS, self, EVENT_CALLBACK(EditView_onSequencerProgress), sizeof(float));
    Event_subscribe(EVENT_NOTE_ADDED, self, EVENT_CALLBACK(EditView_onNoteAdded), sizeof(Note));
    Event_subscribe(EVENT_NOTE_REMOVED, self, EVENT_CALLBACK(EditView_onNoteRemoved), sizeof(Note));
    Event_subscribe(EVENT_ACTIVE_BLOCK_CHANGED, self, EVENT_CALLBACK(EditView_onActiveBlockChanged), sizeof(Color));
    Event_subscribe(EVENT_BLOCK_COLOR_CHANGED, self, EVENT_CALLBACK(EditView_onBlockColorChanged), sizeof(Color));
    Event_subscribe(EVENT_KEY_SIGNATURE_CHANGED, self, EVENT_CALLBACK(EditView_onKeySignatureChanged), sizeof(int));
    Event_subscribe(EVENT_REQUEST_IGNORE_NOTEOFF, self, EVENT_CALLBACK(EditView_onRequestIgnoreNoteoff), sizeof(int));
    Event_subscribe(EVENT_APPLICATION_STATE_CHANGED, self, EVENT_CALLBACK(EditView_onApplicationStateChanged), sizeof(ApplicationState));

    self->state = STATE_INITIALIZING;
    Score_requestCurrentBlockDefNotes(self->score);
    Score_requestCurrentBlockDefColor(self->score);
    Score_requestKeySignature(self->score);

    self->state = STATE_IDLE;

    EditView_unhide(self);

    return self;
}


void EditView_free(EditView** pself) {
    EditView* self = *pself;

    EditView_hide(self);

    Event_unsubscribe(EVENT_CHAR_INPUT, self, EVENT_CALLBACK(EditView_onCharEvent), sizeof(CharEvent));
    Event_unsubscribe(EVENT_KEY_INPUT, self, EVENT_CALLBACK(EditView_onKeyEvent), sizeof(KeyEvent));
    Event_unsubscribe(EVENT_MOUSE_BUTTON_INPUT, self, EVENT_CALLBACK(EditView_onMouseButtonEvent), sizeof(MouseButtonEvent));
    Event_unsubscribe(EVENT_MOUSE_MOTION_INPUT, self, EVENT_CALLBACK(EditView_onMouseMotionEvent), sizeof(Vector2));
    Event_unsubscribe(EVENT_MOUSE_SCROLLWHEEL_INPUT, self, EVENT_CALLBACK(EditView_onMouseScrollwheelEvent), sizeof(Vector2i));
    Event_unsubscribe(EVENT_SEQUENCER_STARTED, self, EVENT_CALLBACK(EditView_onSequencerStarted), sizeof(SequencerRequest));
    Event_unsubscribe(EVENT_SEQUENCER_STOPPED, self, EVENT_CALLBACK(EditView_onSequencerStopped), 0);
    Event_unsubscribe(EVENT_SEQUENCER_PROGRESS, self, EVENT_CALLBACK(EditView_onSequencerProgress), sizeof(float));
    Event_unsubscribe(EVENT_NOTE_ADDED, self, EVENT_CALLBACK(EditView_onNoteAdded), sizeof(Note));
    Event_unsubscribe(EVENT_NOTE_REMOVED, self, EVENT_CALLBACK(EditView_onNoteRemoved), sizeof(Note));
    Event_unsubscribe(EVENT_ACTIVE_BLOCK_CHANGED, self, EVENT_CALLBACK(EditView_onActiveBlockChanged), sizeof(Color));
    Event_unsubscribe(EVENT_BLOCK_COLOR_CHANGED, self, EVENT_CALLBACK(EditView_onBlockColorChanged), sizeof(Color));
    Event_unsubscribe(EVENT_KEY_SIGNATURE_CHANGED, self, EVENT_CALLBACK(EditView_onKeySignatureChanged), sizeof(int));
    Event_unsubscribe(EVENT_REQUEST_IGNORE_NOTEOFF, self, EVENT_CALLBACK(EditView_onRequestIgnoreNoteoff), sizeof(int));
    Event_unsubscribe(EVENT_APPLICATION_STATE_CHANGED, self, EVENT_CALLBACK(EditView_onApplicationStateChanged), sizeof(ApplicationState));

    Grid_free(&self->gridlinesGrid);
    Grid_free(&self->notesGrid);
    Grid_free(&self->ghostNotesGrid);
    Grid_free(&self->cursorGrid);
    Grid_free(&self->dragNoteGrid);
    HashMap_free(&self->noteQuadMap);
    HashMap_free(&self->spatialQuadMap);

    sfree((void**)pself);
}


static void EditView_onCharEvent(EditView* self, void* sender, CharEvent* event) {
    (void)sender;

    if (self->applicationState != APPLICATION_STATE_EDIT_MODE) {
        return;
    }

    CharEvent* eventSave = &(CharEvent){INPUT_CHAR_W, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    CharEvent* eventChangeSynthInstrument = &(CharEvent){INPUT_CHAR_I, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    CharEvent* eventChangeKeySignature = &(CharEvent){INPUT_CHAR_K, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    CharEvent* eventChangeActiveBlock = &(CharEvent){INPUT_CHAR_B, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    CharEvent* eventChangeBlockColor = &(CharEvent){INPUT_CHAR_C, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    CharEvent* eventRenameBlock = &(CharEvent){INPUT_CHAR_R, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    CharEvent* eventSetTempo = &(CharEvent){INPUT_CHAR_T, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    CharEvent* eventIgnoreNoteOff = &(CharEvent){INPUT_CHAR_N, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    CharEvent* eventQuit = &(CharEvent){INPUT_CHAR_Q, INPUT_ACTION_PRESS, INPUT_NO_MODS};

    switch (self->state) {
        case STATE_IDLE:;
            if (charEventMatches(event, eventSave)) {
                Score_saveToFile(self->score);
            } else if (charEventMatches(event, eventChangeSynthInstrument)) {
                int iChannel = NOTE_PREVIEW_MIDI_CHANNEL;
                Event_post(self, EVENT_REQUEST_CHANGE_SYNTH_INSTRUMENT_QUERY, &iChannel, sizeof(iChannel));
            } else if (charEventMatches(event, eventChangeKeySignature)) {
                Score_changeKeySignature(self->score);
            } else if (charEventMatches(event, eventChangeActiveBlock)) {
                Score_changeActiveBlockDef(self->score);
            } else if (charEventMatches(event, eventChangeBlockColor)) {
                Score_changeCurrentBlockDefColor(self->score);
            } else if (charEventMatches(event, eventRenameBlock)) {
                Score_renameCurrentBlockDef(self->score);
            } else if (charEventMatches(event, eventSetTempo)) {
                Score_setTempoBpm(self->score);
            } else if (charEventMatches(event, eventIgnoreNoteOff)) {
                self->ignoreNoteOff = !self->ignoreNoteOff;
                Log_info("Ignore note off: %s", self->ignoreNoteOff ? "true" : "false");
            } else if (charEventMatches(event, eventQuit)) {
                int exitCode = 0;
                Event_post(self, EVENT_REQUEST_QUIT, &exitCode, sizeof(exitCode));
            }
            break;
        default:;
    }
}


static void EditView_onKeyEvent(EditView* self, void* sender, KeyEvent* event) {
    (void)sender;

    if (self->applicationState != APPLICATION_STATE_EDIT_MODE) {
        return;
    }

    KeyEvent* eventPlayBlock = &(KeyEvent){INPUT_KEY_SPACE, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    KeyEvent* eventPlayBlockFromCursor = &(KeyEvent){INPUT_KEY_SPACE, INPUT_ACTION_PRESS, INPUT_MOD_CONTROL};
    KeyEvent* eventPlayBlockLoop = &(KeyEvent){INPUT_KEY_SPACE, INPUT_ACTION_PRESS, INPUT_MOD_SHIFT};
    KeyEvent* eventPlayBlockFromCursorLoop = &(KeyEvent){INPUT_KEY_SPACE, INPUT_ACTION_PRESS, INPUT_MOD_CONTROL | INPUT_MOD_SHIFT};
    KeyEvent* eventToggleActiveBlock = &(KeyEvent){INPUT_KEY_TAB, INPUT_ACTION_PRESS, INPUT_MOD_CONTROL};
    KeyEvent* eventZoomKeyPressed = &(KeyEvent){INPUT_KEY_LEFT_CONTROL, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    KeyEvent* eventZoomKeyReleased = &(KeyEvent){INPUT_KEY_LEFT_CONTROL, INPUT_ACTION_RELEASE, INPUT_MOD_CONTROL};

    if (keyEventMatches(event, eventZoomKeyPressed)) {
        self->isZoomKeyPressed = true;
        return;
    } else if (keyEventMatches(event, eventZoomKeyReleased)){
        self->isZoomKeyPressed = false;
        return;
    }

    switch (self->state) {
        case STATE_IDLE:;
            if (keyEventMatches(event, eventPlayBlock)) {
                Score_playCurrentBlockDef(self->score, NOTE_PREVIEW_MIDI_CHANNEL, 0.0f, self->ignoreNoteOff);
            } else if (keyEventMatches(event, eventPlayBlockFromCursor)) {
                float startTimeFraction = (float)self->cursorPosition.x / (float)getGridSize().x;
                Score_playCurrentBlockDef(self->score, NOTE_PREVIEW_MIDI_CHANNEL, startTimeFraction, self->ignoreNoteOff);
            } else if (keyEventMatches(event, eventPlayBlockLoop)) {
                self->isPlayingLoop = true;
                Score_playCurrentBlockDef(self->score, NOTE_PREVIEW_MIDI_CHANNEL, 0.0f, self->ignoreNoteOff);
            } else if (keyEventMatches(event, eventPlayBlockFromCursorLoop)) {
                self->isPlayingLoop = true;
                float startTimeFraction = (float)self->cursorPosition.x / (float)getGridSize().x;
                Score_playCurrentBlockDef(self->score, NOTE_PREVIEW_MIDI_CHANNEL, startTimeFraction, self->ignoreNoteOff);
            } else if (keyEventMatches(event, eventToggleActiveBlock)) {
                Score_toggleActiveBlockDef(self->score);
            }
            break;
        case STATE_PLAYING:;
            if (keyEventMatches(event, eventPlayBlock)
                    || keyEventMatches(event, eventPlayBlockFromCursor)
                    || keyEventMatches(event, eventPlayBlockLoop)
                    || keyEventMatches(event, eventPlayBlockFromCursorLoop)) {
                self->isPlayingLoop = false;
                Score_stopPlaying(self->score);
            }
            break;
        default:;
    }
}


static void EditView_onMouseButtonEvent(EditView* self, void* sender, MouseButtonEvent* event) {
    (void)self;
    (void)sender;

    if (self->applicationState != APPLICATION_STATE_EDIT_MODE) {
        return;
    }

    MouseButtonEvent* eventStartAddNote = &(MouseButtonEvent){INPUT_MOUSE_BUTTON_LEFT, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    MouseButtonEvent* eventRemoveNote = &(MouseButtonEvent){INPUT_MOUSE_BUTTON_RIGHT, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    MouseButtonEvent* eventStopRemovingNote = &(MouseButtonEvent){INPUT_MOUSE_BUTTON_RIGHT, INPUT_ACTION_RELEASE, INPUT_NO_MODS};
    MouseButtonEvent* eventPreviewNote = &(MouseButtonEvent){INPUT_MOUSE_BUTTON_MIDDLE, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    MouseButtonEvent* eventStopPreviewingNote = &(MouseButtonEvent){INPUT_MOUSE_BUTTON_MIDDLE, INPUT_ACTION_RELEASE, INPUT_NO_MODS};
    MouseButtonEvent* eventStopAddNote = &(MouseButtonEvent){INPUT_MOUSE_BUTTON_LEFT, INPUT_ACTION_RELEASE, INPUT_NO_MODS};
    MouseButtonEvent* eventCancelAddNote = &(MouseButtonEvent){INPUT_MOUSE_BUTTON_RIGHT, INPUT_ACTION_PRESS, INPUT_NO_MODS};

    switch (self->state) {
        case STATE_IDLE:;
            if (mouseButtonEventMatches(event, eventStartAddNote)) {
                self->cursorPositionDragStart = self->cursorPosition;
                self->state = STATE_DRAGGING;
                Grid_hideQuad(self->cursorGrid, self->cursorHandle);
                Grid_updateQuadPosition(self->dragNoteGrid, self->dragNoteHandle, self->cursorPositionDragStart);
                Grid_updateQuadSize(self->dragNoteGrid, self->dragNoteHandle, (Vector2i){1, 1});
                Grid_unhideQuad(self->dragNoteGrid, self->dragNoteHandle);
                EditView_logNoteAction(self, NOTE_ACTION_ADD);
                EditView_previewNoteAtCursor(self);
            } else if (mouseButtonEventMatches(event, eventRemoveNote)) {
                self->isNoteRemovalButtonPressed = true;
                EditView_removeNoteAtCursor(self);
            } else if (mouseButtonEventMatches(event, eventStopRemovingNote)) {
                self->isNoteRemovalButtonPressed = false;
            } else if (mouseButtonEventMatches(event, eventPreviewNote)) {
                self->isNotePreviewButtonPressed = true;
                EditView_logNoteAction(self, NOTE_ACTION_PREVIEW);
                EditView_previewNoteAtCursor(self);
            } else if (mouseButtonEventMatches(event, eventStopPreviewingNote)) {
                self->isNotePreviewButtonPressed = false;
                EditView_stopPreviewingAllNotes(self);
            }
            break;
        case STATE_DRAGGING:;
            if (mouseButtonEventMatches(event, eventStopAddNote)) {
                EditView_addNoteAtCursor(self);
                self->state = STATE_IDLE;
                Grid_hideQuad(self->dragNoteGrid, self->dragNoteHandle);
                EditView_stopPreviewingAllNotes(self);
            } else if (mouseButtonEventMatches(event, eventCancelAddNote)) {
                self->state = STATE_IDLE;
                Grid_hideQuad(self->dragNoteGrid, self->dragNoteHandle);
                EditView_stopPreviewingAllNotes(self);
            }
            break;
        default:;
    }
}


static void EditView_onMouseMotionEvent(EditView* self, void* sender, Vector2* position) {
    (void)sender;

    if (self->applicationState != APPLICATION_STATE_EDIT_MODE) {
        return;
    }

    self->mousePosition = *position;
    self->cursorPosition.x = self->mousePosition.x * getGridSize().x;
    self->cursorPosition.y = self->mousePosition.y * self->nNotesToShow + self->scrollY;
    self->zoomTargetCursorPosition = self->cursorPosition;

    bool cursorUpdated = Grid_updateQuadPosition(self->cursorGrid, self->cursorHandle, self->cursorPosition);
    if (cursorUpdated) {
        EditView_onCursorPositionUpdated(self);
    }

    self->cursorPositionPrev = self->cursorPosition;
}


static void EditView_onMouseScrollwheelEvent(EditView* self, void* sender, Vector2i* offset) {
    (void)sender;

    if (self->applicationState != APPLICATION_STATE_EDIT_MODE) {
        return;
    }

    switch (self->state) {
        case STATE_IDLE:;
        case STATE_PLAYING:;
            if (self->isZoomKeyPressed) {
                self->nNotesToShow = Math_clampi(self->nNotesToShow - offset->y, N_NOTES_TO_SHOW_MIN, N_NOTES_TO_SHOW_MAX);
                self->scrollY = Math_clampi(self->zoomTargetCursorPosition.y - self->mousePosition.y * self->nNotesToShow, 0, MIDI_MESSAGE_PITCH_COUNT - self->nNotesToShow);
            } else {
                self->scrollY = Math_clampi(self->scrollY - offset->y, 0, MIDI_MESSAGE_PITCH_COUNT - self->nNotesToShow);
                self->zoomTargetCursorPosition = self->cursorPosition;
            }
            Grid_zoomTo(self->gridlinesGrid, (Vector2i){0, self->scrollY}, (Vector2i){getGridSize().x, self->nNotesToShow});
            Grid_zoomTo(self->notesGrid, (Vector2i){0, self->scrollY}, (Vector2i){getGridSize().x, self->nNotesToShow});
            Grid_zoomTo(self->ghostNotesGrid, (Vector2i){0, self->scrollY}, (Vector2i){getGridSize().x, self->nNotesToShow});
            Grid_zoomTo(self->cursorGrid, (Vector2i){0, self->scrollY}, (Vector2i){getGridSize().x, self->nNotesToShow});
            Grid_zoomTo(self->dragNoteGrid, (Vector2i){0, self->scrollY}, (Vector2i){getGridSize().x, self->nNotesToShow});

            self->cursorPosition.x = self->mousePosition.x * getGridSize().x;
            self->cursorPosition.y = self->mousePosition.y * self->nNotesToShow + self->scrollY;

            bool cursorUpdated = Grid_updateQuadPosition(self->cursorGrid, self->cursorHandle, self->cursorPosition);
            if (cursorUpdated) {
                EditView_onCursorPositionUpdated(self);
            }

            self->cursorPositionPrev = self->cursorPosition;

            break;
        default:;
    }
}


static void EditView_onNoteAdded(EditView* self, void* sender, Note* note) {
    (void)sender;

    Vector2i position = {
        note->time * getGridSize().x,
        transformNotePitchToRowIndex(note->pitch),
    };
    Vector2i size = {
        note->duration * (float)getGridSize().x,
        1,
    };

    if (self->state == STATE_DRAGGING || self->state == STATE_INITIALIZING) {
        QuadHandle noteQuadHandle = Grid_addQuad(self->notesGrid, position, size, self->blockColor);
        HashMap_addItem(self->noteQuadMap, note, (void*)noteQuadHandle);

        for (int x = position.x; x < position.x + size.x; x++) {
            HashMap_addItem(self->spatialQuadMap, &(Vector2i){x, position.y}, (void*)noteQuadHandle);
        }
    } else if (self->state == STATE_INITIALIZING_GHOST) {
        Grid_addQuad(self->ghostNotesGrid, position, size, GHOST_NOTE_COLOR);
    } else {
        Log_fatal("Expected state to be %d, %d or %d, but was %d", STATE_DRAGGING, STATE_INITIALIZING, STATE_INITIALIZING_GHOST, self->state);
    }
}


static void EditView_onNoteRemoved(EditView* self, void* sender, Note* note) {
    (void)sender;
    Log_assert(self->state == STATE_IDLE || self->state == STATE_DRAGGING, "Expected state to be %d or %d, but was %d", STATE_IDLE, STATE_DRAGGING, self->state);

    EditView_logNoteAction(self, NOTE_ACTION_REMOVE);
    QuadHandle noteQuadHandle = (QuadHandle)HashMap_getItem(self->noteQuadMap, note);

    Vector2i position = {
        note->time * getGridSize().x,
        transformNotePitchToRowIndex(note->pitch),
    };
    Vector2i size = {
        note->duration * (float)getGridSize().x,
        1,
    };

    for (int x = position.x; x < position.x + size.x; x++) {
        HashMap_addItem(self->spatialQuadMap, &(Vector2i){x, position.y}, NULL);
    }

    Grid_hideQuad(self->notesGrid, noteQuadHandle);
}


static void EditView_onActiveBlockChanged(EditView* self, void* sender, Color* color) {
    (void)sender;
    self->blockColor = *color;
    EditView_refreshNoteGrid(self);
}


static void EditView_onBlockColorChanged(EditView* self, void* sender, Color* color) {
    (void)sender;
    self->blockColor = *color;
    EditView_refreshNoteGrid(self);
}


static void EditView_onKeySignatureChanged(EditView* self, void* sender, int* iKeySignature) {
    (void)sender;
    for (int i = 0; i < MIDI_MESSAGE_PITCH_COUNT; i++) {
        int note = Math_posmod(MIDI_MESSAGE_PITCH_COUNT - 1 - i, N_NOTES_IN_OCTAVE);
        Color color = KEY_SIGNATURES[*iKeySignature][note] ? GRID_LINE_COLOR : BACKGROUND_COLOR;
        Grid_updateQuadColor(self->gridlinesGrid, self->horizontalGridLineHandles[i], color);
    }
}


static void EditView_onRequestIgnoreNoteoff(EditView* self, void* sender, int* ignoreNoteOff) {
    (void)sender;
    self->ignoreNoteOff = *ignoreNoteOff;
}


static void EditView_onSequencerStarted(EditView* self, void* sender, SequencerRequest* sequencerRequest) {
    (void)sender; (void)sequencerRequest;

    if (self->applicationState != APPLICATION_STATE_EDIT_MODE) {
        return;
    }

    Grid_hideQuad(self->cursorGrid, self->cursorHandle);


    QuadHandle quadHandle = (QuadHandle)HashMap_getItem(self->spatialQuadMap, &self->cursorPosition);
    if (quadHandle) {
        Grid_updateQuadColor(self->notesGrid, quadHandle, self->blockColor);
    }

    self->playbackCursorPosition = (Vector2i){0, 0};
    Grid_updateQuadPosition(self->cursorGrid, self->playbackCursorHandle, self->playbackCursorPosition);

    Color cursorInitialColor = lerpColor(CURSOR_COLOR, HIGHLIGHT_COLOR, PLAYBACK_CURSOR_HIGHLIGHT_STRENGTH);
    Grid_updateQuadColor(self->cursorGrid, self->playbackCursorHandle, cursorInitialColor);
    Grid_animateQuadColor(self->cursorGrid, self->playbackCursorHandle, CURSOR_COLOR, PLAYBACK_CURSOR_HIGHLIGHT_LERP_WEIGHT);

    self->state = STATE_PLAYING;
}


static void EditView_onSequencerStopped(EditView* self, void* sender, void* unused) {
    (void)sender; (void)unused;

    for (int iPitch = 0; iPitch < MIDI_MESSAGE_PITCH_COUNT; iPitch++) {
        Vector2i position = {self->playbackCursorPosition.x, iPitch};
        QuadHandle quadHandle = (QuadHandle)HashMap_getItem(self->spatialQuadMap, &position);
        if (quadHandle) {
            Grid_updateQuadColor(self->notesGrid, quadHandle, self->blockColor);
        }
    }

    if (self->isPlayingLoop) {
        Score_playCurrentBlockDef(self->score, NOTE_PREVIEW_MIDI_CHANNEL, 0.0f, self->ignoreNoteOff);
    } else {
        Grid_hideQuad(self->cursorGrid, self->playbackCursorHandle);
        Grid_unhideQuad(self->cursorGrid, self->cursorHandle);

        QuadHandle quadHandle = (QuadHandle)HashMap_getItem(self->spatialQuadMap, &self->cursorPosition);
        if (quadHandle) {
            Color noteHighlightColor = lerpColor(self->blockColor, HIGHLIGHT_COLOR, NOTE_HIGHLIGHT_STRENGTH);
            Grid_updateQuadColor(self->notesGrid, quadHandle, noteHighlightColor);
        }

        self->state = STATE_IDLE;
    }
}


static void EditView_onSequencerProgress(EditView* self, void* sender, float* progress) {
    (void)sender;

    if (self->applicationState != APPLICATION_STATE_EDIT_MODE) {
        return;
    }

    Log_assert(self->state == STATE_PLAYING, "Unexpected sequencer progress update in state %d", self->state);

    Grid_unhideQuad(self->cursorGrid, self->playbackCursorHandle);
    self->playbackCursorPosition = (Vector2i){*progress * (float)getGridSize().x, 0};
    bool cursorUpdated = Grid_updateQuadPosition(self->cursorGrid, self->playbackCursorHandle, self->playbackCursorPosition);

    if (cursorUpdated) {
        for (int iPitch = 0; iPitch < MIDI_MESSAGE_PITCH_COUNT; iPitch++) {
            Vector2i position = {self->playbackCursorPosition.x, iPitch};
            Vector2i positionPrev = {self->playbackCursorPositionPrev.x, iPitch};
            QuadHandle quadHandle = (QuadHandle)HashMap_getItem(self->spatialQuadMap, &position);
            QuadHandle quadHandlePrev = (QuadHandle)HashMap_getItem(self->spatialQuadMap, &positionPrev);
            if (quadHandle != quadHandlePrev) {
                Color noteHighlightColor = lerpColor(self->blockColor, HIGHLIGHT_COLOR, NOTE_HIGHLIGHT_STRENGTH);
                if (quadHandle) {
                    Grid_updateQuadColor(self->notesGrid, quadHandle, noteHighlightColor);
                }
                if (quadHandlePrev) {
                    Grid_updateQuadColor(self->notesGrid, quadHandlePrev, self->blockColor);
                }
            }
        }
    } else if (self->playbackCursorPosition.x == 0) {
        for (int iPitch = 0; iPitch < MIDI_MESSAGE_PITCH_COUNT; iPitch++) {
            Vector2i position = {self->playbackCursorPosition.x, iPitch};
            QuadHandle quadHandle = (QuadHandle)HashMap_getItem(self->spatialQuadMap, &position);
            Color noteHighlightColor = lerpColor(self->blockColor, HIGHLIGHT_COLOR, NOTE_HIGHLIGHT_STRENGTH);
            if (quadHandle) {
                Grid_updateQuadColor(self->notesGrid, quadHandle, noteHighlightColor);
            }
        }
    }

    self->playbackCursorPositionPrev = self->playbackCursorPosition;
}


static void EditView_onApplicationStateChanged(EditView* self, void* sender, ApplicationState* applicationState) {
    (void)sender;

    self->applicationState = *applicationState;

    if (self->applicationState == APPLICATION_STATE_EDIT_MODE) {
        EditView_unhide(self);
    } else {
        EditView_hide(self);
    }
}


static void EditView_onCursorPositionUpdated(EditView* self) {
    switch (self->state) {
        case STATE_IDLE:;
            Grid_unhideQuad(self->cursorGrid, self->cursorHandle);
            if (self->isNotePreviewButtonPressed) {
                EditView_stopPreviewingAllNotes(self);
                EditView_logNoteAction(self, NOTE_ACTION_PREVIEW);
                EditView_previewNoteAtCursor(self);
            }
            if (self->isNoteRemovalButtonPressed) {
                // Make sure that notes are not skipped over when moving the cursor quickly
                Vector2i removalPosition = self->cursorPositionPrev;
                while(removalPosition.x != self->cursorPosition.x || removalPosition.y != self->cursorPosition.y) {
                    if (Math_abs(removalPosition.x - self->cursorPosition.x) > Math_abs(removalPosition.y - self->cursorPosition.y)) {
                        removalPosition.x = Math_max(removalPosition.x - 1, Math_min(removalPosition.x + 1, self->cursorPosition.x));
                    } else {
                        removalPosition.y = Math_max(removalPosition.y - 1, Math_min(removalPosition.y + 1, self->cursorPosition.y));
                    }
                    EditView_removeNoteAtPosition(self, removalPosition);
                }
            }
            Grid_hideQuad(self->dragNoteGrid, self->dragNoteHandle);

            QuadHandle quadHandle = (QuadHandle)HashMap_getItem(self->spatialQuadMap, &self->cursorPosition);
            QuadHandle quadHandlePrev = (QuadHandle)HashMap_getItem(self->spatialQuadMap, &self->cursorPositionPrev);
            if (quadHandle != quadHandlePrev) {
                Color noteHighlightColor = lerpColor(self->blockColor, HIGHLIGHT_COLOR, NOTE_HIGHLIGHT_STRENGTH);
                if (quadHandle) {
                    Grid_updateQuadColor(self->notesGrid, quadHandle, noteHighlightColor);
                }
                if (quadHandlePrev) {
                    Grid_updateQuadColor(self->notesGrid, quadHandlePrev, self->blockColor);
                }
            }

            break;
        case STATE_DRAGGING:;
            int dragNoteLength = Math_max(self->cursorPosition.x - self->cursorPositionDragStart.x + 1, 1);
            Grid_updateQuadSize(self->dragNoteGrid, self->dragNoteHandle, (Vector2i){dragNoteLength, 1});
            Grid_hideQuad(self->cursorGrid, self->cursorHandle);
            break;
        default:;
    }
}


static void EditView_addNoteAtCursor(EditView* self) {
    int pitch = transformRowIndexToNotePitch(self->cursorPositionDragStart.y);
    float time = (float)self->cursorPositionDragStart.x / (float)getGridSize().x;
    int dragNoteLength = Math_max(self->cursorPosition.x - self->cursorPositionDragStart.x + 1, 1);
    float duration = dragNoteLength / (float)getGridSize().x;
    float velocity = NOTE_VELOCITY_DEFAULT;
    Score_addNote(self->score, pitch, time, duration, velocity);
}


static void EditView_removeNoteAtCursor(EditView* self) {
    EditView_removeNoteAtPosition(self, self->cursorPosition);
}


static void EditView_removeNoteAtPosition(EditView* self, Vector2i position) {
    int pitch = transformRowIndexToNotePitch(position.y);
    float time = (float)position.x / (float)getGridSize().x;
    Score_removeNotes(self->score, pitch, time, time + 1.0f / getGridSize().x);
}


static void EditView_previewNoteAtCursor(EditView* self) {
    MidiMessage midiMessage = {
        .type = MIDI_MESSAGE_TYPE_NOTEON,
        .channel = NOTE_PREVIEW_MIDI_CHANNEL,
        .pitch = transformRowIndexToNotePitch(self->cursorPosition.y),
        .velocity = (float)MIDI_MESSAGE_VELOCITY_MAX * NOTE_PREVIEW_VELOCITY,
    };
    Event_post(self, EVENT_REQUEST_MIDI_MESSAGE_PLAY, &midiMessage, sizeof(midiMessage));
}


static void EditView_stopPreviewingAllNotes(EditView* self) {
    int iChannel = NOTE_PREVIEW_MIDI_CHANNEL;
    Event_post(self, EVENT_REQUEST_MIDI_CHANNEL_STOP, &iChannel, sizeof(iChannel));
}


static void EditView_logNoteAction(EditView* self, const char* const action) {
    int pitch = transformRowIndexToNotePitch(self->cursorPosition.y);
    Log_info("%s note %s%d (pitch %d)", action, NOTE_NAMES[pitch % N_NOTES_IN_OCTAVE], pitch / N_NOTES_IN_OCTAVE - 1, pitch);
}


static void EditView_refreshNoteGrid(EditView* self) {
    Grid_free(&self->notesGrid);
    self->notesGrid = Grid_new(getGridSize(), (Vector2){NOTES_GRID_CELL_SPACING, NOTES_GRID_CELL_SPACING});
    Grid_zoomTo(self->notesGrid, (Vector2i){0, self->scrollY}, (Vector2i){getGridSize().x, self->nNotesToShow});
    Grid_hide(self->notesGrid);

    Grid_free(&self->ghostNotesGrid);
    self->ghostNotesGrid = Grid_new(getGridSize(), (Vector2){NOTES_GRID_CELL_SPACING, NOTES_GRID_CELL_SPACING});
    Grid_zoomTo(self->ghostNotesGrid, (Vector2i){0, self->scrollY}, (Vector2i){getGridSize().x, self->nNotesToShow});
    Grid_hide(self->ghostNotesGrid);

    HashMap_clear(self->noteQuadMap);

    HashMap_clear(self->spatialQuadMap);
    for (int x = 0; x < getGridSize().x; x++) {
        for (int y = 0; y < getGridSize().y; y++) {
            Vector2i position = {x, y};
            HashMap_addItem(self->spatialQuadMap, &position, NULL);
        }
    }

    Color dragNoteColor = lerpColor(self->blockColor, HIGHLIGHT_COLOR, NOTE_HIGHLIGHT_STRENGTH);
    Grid_updateQuadColor(self->dragNoteGrid, self->dragNoteHandle, dragNoteColor);

    self->state = STATE_INITIALIZING;
    Score_requestCurrentBlockDefNotes(self->score);

    self->state = STATE_INITIALIZING_GHOST;
    Score_requestPrevBlockDefNotes(self->score);

    self->state = STATE_IDLE;

    // Fix z-ordering
    if (self->applicationState == APPLICATION_STATE_EDIT_MODE) {
        EditView_hideGrids(self);
        EditView_unhideGrids(self);
    }
}


static void EditView_hide(EditView* self) {
    self->isPlayingLoop = false;
    Score_stopPlaying(self->score);

    EditView_hideGrids(self);
}


static void EditView_hideGrids(EditView* self) {
    Grid_hide(self->gridlinesGrid);
    Grid_hide(self->ghostNotesGrid);
    Grid_hide(self->cursorGrid);
    Grid_hide(self->dragNoteGrid);
    Grid_hide(self->notesGrid);
}


static void EditView_unhide(EditView* self) {
    // Hide and then unhide, to make sure the grids are all added in the correct z-order.
    EditView_hideGrids(self);
    EditView_unhideGrids(self);
}


static void EditView_unhideGrids(EditView* self) {
    Grid_unhide(self->gridlinesGrid);
    Grid_unhide(self->ghostNotesGrid);
    Grid_unhide(self->cursorGrid);
    Grid_unhide(self->dragNoteGrid);
    Grid_unhide(self->notesGrid);
}


static int transformRowIndexToNotePitch(int iRow) {
    return MIDI_MESSAGE_VELOCITY_MAX - iRow;
}


static int transformNotePitchToRowIndex(int pitch) {
    return MIDI_MESSAGE_PITCH_MAX - pitch;
}


static Vector2i getGridSize(void) {
    return (Vector2i){N_BEATS_TOTAL * MEASURE_RESOLUTION, MIDI_MESSAGE_PITCH_COUNT};
}
