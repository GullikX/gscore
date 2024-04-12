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

#include "objectview.h"

#include "common/constants/applicationstate.h"
#include "common/constants/fluidmidi.h"
#include "common/constants/input.h"
#include "common/structs/blockinstance.h"
#include "common/structs/color.h"
#include "common/structs/keyevent.h"
#include "common/structs/mousebuttonevent.h"
#include "common/structs/sequencerrequest.h"
#include "common/structs/vector2i.h"
#include "common/util/alloc.h"
#include "common/util/colors.h"
#include "common/util/inputmatcher.h"
#include "common/util/hashmap.h"
#include "common/util/log.h"
#include "common/util/math.h"
#include "common/visual/grid/grid.h"
#include "config/config.h"
#include "events/events.h"


static const float PLAYBACK_BLOCK_HIGHLIGHT_LERP_WEIGHT = 2.0f;
static const Vector2i CURSOR_SIZE = {1, 1};


typedef enum {
    STATE_INVALID,
    STATE_IDLE,
    STATE_PLAYING,
    STATE_INITIALIZING,
} State;


struct ObjectView {
    Score* score;
    State state;
    ApplicationState applicationState;
    Grid* gridlinesGrid;
    Grid* cursorGrid;
    Grid* blocksGrid;
    Color blockColor;
    QuadHandle cursorHandle;
    QuadHandle playbackCursorHandle;
    Vector2 mousePosition;
    Vector2i cursorPosition;
    Vector2i cursorPositionPrev;
    Vector2i playbackCursorPosition;
    Vector2i playbackCursorPositionPrev;
    HashMap* spatialQuadMap;
    HashMap* spatialBlockInstanceMap;
    bool isBlockAddButtonPressed;
    bool isBlockRemovalButtonPressed;
    bool isZoomKeyPressed;
    int scrollX;
    int nBlocksToShow;
    int nBlocksToShowInit;
    Vector2i zoomTargetCursorPosition;
    SequencerRequest* sequencerRequest;
    size_t iPlaybackMidiMessage;
};


static void ObjectView_onCharEvent(ObjectView* self, void* sender, CharEvent* event);
static void ObjectView_onKeyEvent(ObjectView* self, void* sender, KeyEvent* event);
static void ObjectView_onMouseButtonEvent(ObjectView* self, void* sender, MouseButtonEvent* event);
static void ObjectView_onMouseMotionEvent(ObjectView* self, void* sender, Vector2* position);
static void ObjectView_onMouseScrollwheelEvent(ObjectView* self, void* sender, Vector2i* offset);
static void ObjectView_onBlockInstanceAdded(ObjectView* self, void* sender, BlockInstance* blockInstance);
static void ObjectView_onBlockInstanceRemoved(ObjectView* self, void* sender, BlockInstance* blockInstance);
static void ObjectView_onActiveBlockChanged(ObjectView* self, void* sender, Color* color);
static void ObjectView_onBlockColorChanged(ObjectView* self, void* sender, Color* color);
static void ObjectView_onSequencerStarted(ObjectView* self, void* sender, SequencerRequest* sequencerRequest);
static void ObjectView_onSequencerStopped(ObjectView* self, void* sender, void* unused);
static void ObjectView_onSequencerProgress(ObjectView* self, void* sender, float* progress);
static void ObjectView_onApplicationStateChanged(ObjectView* self, void* sender, ApplicationState* applicationState);
static void ObjectView_onCursorPositionUpdated(ObjectView* self);
static void ObjectView_addBlockInstanceAtCursor(ObjectView* self);
static void ObjectView_addBlockInstanceAtPosition(ObjectView* self, Vector2i position);
static void ObjectView_removeBlockInstanceAtCursor(ObjectView* self);
static void ObjectView_removeBlockInstanceAtPosition(ObjectView* self, Vector2i position);
static void ObjectView_hide(ObjectView* self);
static void ObjectView_unhide(ObjectView* self);
static void ObjectView_refreshBlockGrid(ObjectView* self);
static Vector2i getGridSize(void);


ObjectView* ObjectView_new(Score* score) {
    ObjectView* self = ecalloc(1, sizeof(*self));

    self->score = score;
    self->nBlocksToShowInit = SCORE_LENGTH_DEFAULT;
    self->scrollX = 0;

    self->gridlinesGrid = Grid_new(getGridSize(), (Vector2){GRIDLINES_GRID_CELL_SPACING, GRIDLINES_GRID_CELL_SPACING});
    self->cursorGrid = Grid_new(getGridSize(), (Vector2){CURSOR_GRID_CELL_SPACING, CURSOR_GRID_CELL_SPACING});
    self->blocksGrid = Grid_new(getGridSize(), (Vector2){BLOCKS_GRID_CELL_SPACING, BLOCKS_GRID_CELL_SPACING});

    for (int i = 0; i < N_SYNTH_TRACKS; i++) {
        Vector2i position = {0, i};
        Vector2i size = {SCORE_LENGTH_MAX, 1};
        if (i % 2) {
            Grid_addQuad(self->gridlinesGrid, position, size, GRID_LINE_COLOR);
        }
    }

    for (int i = 0; i < SCORE_LENGTH_MAX; i++) {
        Vector2i position = {i, 0};
        Vector2i size = {1, N_SYNTH_TRACKS};
        if (i % 4 == 0) {
            Grid_addQuad(self->gridlinesGrid, position, size, GRID_LINE_COLOR);
        }
    }

    self->cursorHandle = Grid_addQuad(self->cursorGrid, self->cursorPosition, CURSOR_SIZE, CURSOR_COLOR);

    self->playbackCursorHandle = Grid_addQuad(self->cursorGrid, self->cursorPosition, (Vector2i){1, N_SYNTH_TRACKS}, CURSOR_COLOR);
    Grid_hideQuad(self->cursorGrid, self->playbackCursorHandle);

    self->spatialQuadMap = HashMap_new(sizeof(Vector2i));
    self->spatialBlockInstanceMap = HashMap_new(sizeof(Vector2i));
    for (int x = 0; x < getGridSize().x; x++) {
        for (int y = 0; y < getGridSize().y; y++) {
            Vector2i position = {x, y};
            HashMap_addItem(self->spatialQuadMap, &position, NULL);
            HashMap_addItem(self->spatialBlockInstanceMap, &position, NULL);
        }
    }

    Event_subscribe(EVENT_CHAR_INPUT, self, EVENT_CALLBACK(ObjectView_onCharEvent), sizeof(CharEvent));
    Event_subscribe(EVENT_KEY_INPUT, self, EVENT_CALLBACK(ObjectView_onKeyEvent), sizeof(KeyEvent));
    Event_subscribe(EVENT_MOUSE_BUTTON_INPUT, self, EVENT_CALLBACK(ObjectView_onMouseButtonEvent), sizeof(MouseButtonEvent));
    Event_subscribe(EVENT_MOUSE_MOTION_INPUT, self, EVENT_CALLBACK(ObjectView_onMouseMotionEvent), sizeof(Vector2));
    Event_subscribe(EVENT_MOUSE_SCROLLWHEEL_INPUT, self, EVENT_CALLBACK(ObjectView_onMouseScrollwheelEvent), sizeof(Vector2i));
    Event_subscribe(EVENT_SEQUENCER_STARTED, self, EVENT_CALLBACK(ObjectView_onSequencerStarted), sizeof(SequencerRequest));
    Event_subscribe(EVENT_SEQUENCER_STOPPED, self, EVENT_CALLBACK(ObjectView_onSequencerStopped), 0);
    Event_subscribe(EVENT_SEQUENCER_PROGRESS, self, EVENT_CALLBACK(ObjectView_onSequencerProgress), sizeof(float));
    Event_subscribe(EVENT_BLOCK_INSTANCE_ADDED, self, EVENT_CALLBACK(ObjectView_onBlockInstanceAdded), sizeof(BlockInstance));
    Event_subscribe(EVENT_BLOCK_INSTANCE_REMOVED, self, EVENT_CALLBACK(ObjectView_onBlockInstanceRemoved), sizeof(BlockInstance));
    Event_subscribe(EVENT_ACTIVE_BLOCK_CHANGED, self, EVENT_CALLBACK(ObjectView_onActiveBlockChanged), sizeof(Color));
    Event_subscribe(EVENT_BLOCK_COLOR_CHANGED, self, EVENT_CALLBACK(ObjectView_onBlockColorChanged), sizeof(Color));
    Event_subscribe(EVENT_APPLICATION_STATE_CHANGED, self, EVENT_CALLBACK(ObjectView_onApplicationStateChanged), sizeof(ApplicationState));

    self->nBlocksToShow = SCORE_LENGTH_MAX;

    self->state = STATE_INITIALIZING;
    Score_requestCurrentBlockDefColor(self->score); // Will also request block instances

    self->state = STATE_IDLE;

    self->nBlocksToShow = self->nBlocksToShowInit;
    Grid_zoomTo(self->gridlinesGrid, (Vector2i){self->scrollX, 0}, (Vector2i){self->nBlocksToShow, getGridSize().y});
    Grid_zoomTo(self->blocksGrid, (Vector2i){self->scrollX, 0}, (Vector2i){self->nBlocksToShow, getGridSize().y});
    Grid_zoomTo(self->cursorGrid, (Vector2i){self->scrollX, 0}, (Vector2i){self->nBlocksToShow, getGridSize().y,});

    ObjectView_unhide(self);

    return self;
}


void ObjectView_free(ObjectView** pself) {
    ObjectView* self = *pself;

    ObjectView_hide(self);

    Event_unsubscribe(EVENT_CHAR_INPUT, self, EVENT_CALLBACK(ObjectView_onCharEvent), sizeof(CharEvent));
    Event_unsubscribe(EVENT_KEY_INPUT, self, EVENT_CALLBACK(ObjectView_onKeyEvent), sizeof(KeyEvent));
    Event_unsubscribe(EVENT_MOUSE_BUTTON_INPUT, self, EVENT_CALLBACK(ObjectView_onMouseButtonEvent), sizeof(MouseButtonEvent));
    Event_unsubscribe(EVENT_MOUSE_MOTION_INPUT, self, EVENT_CALLBACK(ObjectView_onMouseMotionEvent), sizeof(Vector2));
    Event_unsubscribe(EVENT_MOUSE_SCROLLWHEEL_INPUT, self, EVENT_CALLBACK(ObjectView_onMouseScrollwheelEvent), sizeof(Vector2i));
    Event_unsubscribe(EVENT_SEQUENCER_STARTED, self, EVENT_CALLBACK(ObjectView_onSequencerStarted), sizeof(SequencerRequest));
    Event_unsubscribe(EVENT_SEQUENCER_STOPPED, self, EVENT_CALLBACK(ObjectView_onSequencerStopped), 0);
    Event_unsubscribe(EVENT_SEQUENCER_PROGRESS, self, EVENT_CALLBACK(ObjectView_onSequencerProgress), sizeof(float));
    Event_unsubscribe(EVENT_BLOCK_INSTANCE_ADDED, self, EVENT_CALLBACK(ObjectView_onBlockInstanceAdded), sizeof(BlockInstance));
    Event_unsubscribe(EVENT_BLOCK_INSTANCE_REMOVED, self, EVENT_CALLBACK(ObjectView_onBlockInstanceRemoved), sizeof(BlockInstance));
    Event_unsubscribe(EVENT_ACTIVE_BLOCK_CHANGED, self, EVENT_CALLBACK(ObjectView_onActiveBlockChanged), sizeof(Color));
    Event_unsubscribe(EVENT_BLOCK_COLOR_CHANGED, self, EVENT_CALLBACK(ObjectView_onBlockColorChanged), sizeof(Color));
    Event_unsubscribe(EVENT_APPLICATION_STATE_CHANGED, self, EVENT_CALLBACK(ObjectView_onApplicationStateChanged), sizeof(ApplicationState));

    Grid_free(&self->gridlinesGrid);
    Grid_free(&self->blocksGrid);
    Grid_free(&self->cursorGrid);

    HashMap_free(&self->spatialQuadMap);

    for (Vector2i* position = HashMap_iterateInit(self->spatialBlockInstanceMap); position; position = HashMap_iterateNext(self->spatialBlockInstanceMap, position)) {
        BlockInstance* blockInstance = HashMap_getItem(self->spatialBlockInstanceMap, position);
        if (blockInstance) {
            sfree((void**)&blockInstance);
        }
    }
    HashMap_free(&self->spatialBlockInstanceMap);

    if (self->sequencerRequest) {
        if (self->sequencerRequest->midiMessages) {
            sfree((void**)&self->sequencerRequest->midiMessages);
        }
        sfree((void**)&self->sequencerRequest);
    }

    sfree((void**)pself);
}


static void ObjectView_onCharEvent(ObjectView* self, void* sender, CharEvent* event) {
    (void)sender;

    if (self->applicationState != APPLICATION_STATE_OBJECT_MODE) {
        return;
    }

    CharEvent* eventSave = &(CharEvent){INPUT_CHAR_W, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    CharEvent* eventChangeSynthInstrument = &(CharEvent){INPUT_CHAR_I, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    CharEvent* eventChangeActiveBlock = &(CharEvent){INPUT_CHAR_B, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    CharEvent* eventChangeBlockColor = &(CharEvent){INPUT_CHAR_C, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    CharEvent* eventRenameBlock = &(CharEvent){INPUT_CHAR_R, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    CharEvent* eventSetTempo = &(CharEvent){INPUT_CHAR_T, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    CharEvent* eventIgnoreNoteOff = &(CharEvent){INPUT_CHAR_N, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    CharEvent* eventChangeTrackVelocity = &(CharEvent){INPUT_CHAR_V, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    CharEvent* eventQuit = &(CharEvent){INPUT_CHAR_Q, INPUT_ACTION_PRESS, INPUT_NO_MODS};

    int iTrack = self->cursorPosition.y;
    int iChannel = iTrack + 1;

    switch (self->state) {
        case STATE_IDLE:;
            if (charEventMatches(event, eventSave)) {
                Score_saveToFile(self->score);
            } else if (charEventMatches(event, eventChangeSynthInstrument)) {
                Event_post(self, EVENT_REQUEST_CHANGE_SYNTH_INSTRUMENT_QUERY, &iChannel, sizeof(iChannel));
            } else if (charEventMatches(event, eventChangeActiveBlock)) {
                Score_changeActiveBlockDef(self->score);
            } else if (charEventMatches(event, eventChangeBlockColor)) {
                Score_changeCurrentBlockDefColor(self->score);
            } else if (charEventMatches(event, eventRenameBlock)) {
                Score_renameCurrentBlockDef(self->score);
            } else if (charEventMatches(event, eventSetTempo)) {
                Score_setTempoBpm(self->score);
            } else if (charEventMatches(event, eventIgnoreNoteOff)) {
                Score_toggleIgnoreNoteOff(self->score, iTrack);
            } else if (charEventMatches(event, eventChangeTrackVelocity)) {
                Score_changeTrackVelocity(self->score, iTrack);
            } else if (charEventMatches(event, eventQuit)) {
                int exitCode = 0;
                Event_post(self, EVENT_REQUEST_QUIT, &exitCode, sizeof(exitCode));
            }
            break;
        default:;
    }
}


static void ObjectView_onKeyEvent(ObjectView* self, void* sender, KeyEvent* event) {
    (void)sender;

    if (self->applicationState != APPLICATION_STATE_OBJECT_MODE) {
        return;
    }

    KeyEvent* eventPlayScore = &(KeyEvent){INPUT_KEY_SPACE, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    KeyEvent* eventPlayScoreFromCursor = &(KeyEvent){INPUT_KEY_SPACE, INPUT_ACTION_PRESS, INPUT_MOD_CONTROL};
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
            if (keyEventMatches(event, eventPlayScore)) {
                Score_playEntireScore(self->score, 0);
            } else if (keyEventMatches(event, eventPlayScoreFromCursor)) {
                Score_playEntireScore(self->score, self->cursorPosition.x);
            }
            break;
        case STATE_PLAYING:;
            if (keyEventMatches(event, eventPlayScore) || keyEventMatches(event, eventPlayScoreFromCursor)) {
                Score_stopPlaying(self->score);
            }
            break;
    }
}


static void ObjectView_onMouseButtonEvent(ObjectView* self, void* sender, MouseButtonEvent* event) {
    (void)sender;

    if (self->applicationState != APPLICATION_STATE_OBJECT_MODE) {
        return;
    }

    MouseButtonEvent* eventAddBlock = &(MouseButtonEvent){INPUT_MOUSE_BUTTON_LEFT, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    MouseButtonEvent* eventStopAddingBlock = &(MouseButtonEvent){INPUT_MOUSE_BUTTON_LEFT, INPUT_ACTION_RELEASE, INPUT_NO_MODS};
    MouseButtonEvent* eventRemoveBlock = &(MouseButtonEvent){INPUT_MOUSE_BUTTON_RIGHT, INPUT_ACTION_PRESS, INPUT_NO_MODS};
    MouseButtonEvent* eventStopRemovingBlock = &(MouseButtonEvent){INPUT_MOUSE_BUTTON_RIGHT, INPUT_ACTION_RELEASE, INPUT_NO_MODS};
    MouseButtonEvent* eventPickBlockDef = &(MouseButtonEvent){INPUT_MOUSE_BUTTON_MIDDLE, INPUT_ACTION_PRESS, INPUT_NO_MODS};

    switch (self->state) {
        case STATE_IDLE:;
            if (mouseButtonEventMatches(event, eventAddBlock)) {
                self->isBlockAddButtonPressed = true;
                ObjectView_addBlockInstanceAtCursor(self);
            } else if (mouseButtonEventMatches(event, eventStopAddingBlock)) {
                self->isBlockAddButtonPressed = false;
                QuadHandle quadHandle = (QuadHandle)HashMap_getItem(self->spatialQuadMap, &self->cursorPosition);
                if (quadHandle) {
                    Grid_updateQuadColor(self->blocksGrid, quadHandle, self->blockColor);
                }
            } else if (mouseButtonEventMatches(event, eventRemoveBlock)) {
                self->isBlockRemovalButtonPressed = true;
                ObjectView_removeBlockInstanceAtCursor(self);
            } else if (mouseButtonEventMatches(event, eventStopRemovingBlock)) {
                self->isBlockRemovalButtonPressed = false;
            } else if (mouseButtonEventMatches(event, eventPickBlockDef)) {
                Score_pickBlockDef(self->score, self->cursorPosition.y, self->cursorPosition.x);
            }
            break;
        default:;
    }
}


static void ObjectView_onMouseMotionEvent(ObjectView* self, void* sender, Vector2* position) {
    (void)sender;

    if (self->applicationState != APPLICATION_STATE_OBJECT_MODE) {
        return;
    }

    self->mousePosition = *position;
    self->cursorPosition.x = self->mousePosition.x * self->nBlocksToShow + self->scrollX;
    self->cursorPosition.y = self->mousePosition.y * getGridSize().y;
    self->zoomTargetCursorPosition = self->cursorPosition;

    bool cursorUpdated = Grid_updateQuadPosition(self->cursorGrid, self->cursorHandle, self->cursorPosition);
    if (cursorUpdated) {
        ObjectView_onCursorPositionUpdated(self);
    }

    self->cursorPositionPrev = self->cursorPosition;
}


static void ObjectView_onMouseScrollwheelEvent(ObjectView* self, void* sender, Vector2i* offset) {
    (void)sender;

    if (self->applicationState != APPLICATION_STATE_OBJECT_MODE) {
        return;
    }

    switch (self->state) {
        case STATE_IDLE:;
        case STATE_PLAYING:;
            if (self->isZoomKeyPressed) {
                self->nBlocksToShow = Math_clampi(self->nBlocksToShow - offset->y, SCORE_LENGTH_MIN, SCORE_LENGTH_MAX);
                self->scrollX = Math_clampi(self->zoomTargetCursorPosition.x - self->mousePosition.x * self->nBlocksToShow, 0, SCORE_LENGTH_MAX - self->nBlocksToShow);
            } else {
                self->scrollX = Math_clampi(self->scrollX - offset->y, 0, SCORE_LENGTH_MAX - self->nBlocksToShow);
                self->zoomTargetCursorPosition = self->cursorPosition;
            }
            Grid_zoomTo(self->gridlinesGrid, (Vector2i){self->scrollX, 0}, (Vector2i){self->nBlocksToShow, getGridSize().y});
            Grid_zoomTo(self->blocksGrid, (Vector2i){self->scrollX, 0}, (Vector2i){self->nBlocksToShow, getGridSize().y});
            Grid_zoomTo(self->cursorGrid, (Vector2i){self->scrollX, 0}, (Vector2i){self->nBlocksToShow, getGridSize().y});

            self->cursorPosition.x = self->mousePosition.x * self->nBlocksToShow + self->scrollX;
            self->cursorPosition.y = self->mousePosition.y * getGridSize().y;

            bool cursorUpdated = Grid_updateQuadPosition(self->cursorGrid, self->cursorHandle, self->cursorPosition);
            if (cursorUpdated) {
                ObjectView_onCursorPositionUpdated(self);
            }

            self->cursorPositionPrev = self->cursorPosition;

            break;
        default:;
    }
}


static void ObjectView_onBlockInstanceAdded(ObjectView* self, void* sender, BlockInstance* blockInstance) {
    (void)sender;
    Log_assert(self->state == STATE_IDLE || self->state == STATE_INITIALIZING, "Expected state to be %d or %d, but was %d", STATE_IDLE, STATE_INITIALIZING, self->state);

    Vector2i position = {
        blockInstance->iTimeSlot,
        blockInstance->iTrack,
    };
    Vector2i size = {1, 1};
    Color color = blockInstance->color;
    if (position.x == self->cursorPosition.x && position.y == self->cursorPosition.y) {
        color = lerpColor(color, HIGHLIGHT_COLOR, NOTE_HIGHLIGHT_STRENGTH);
    }
    QuadHandle blockQuadHandle = Grid_addQuad(self->blocksGrid, position, size, color);

    Log_assert(HashMap_getItem(self->spatialQuadMap, &position) == NULL, "Position (%d, %d) already in spatial quad map", position.x, position.y);
    Log_assert(HashMap_getItem(self->spatialBlockInstanceMap, &position) == NULL, "Position (%d, %d) already in spatial block instance map", position.x, position.y);

    HashMap_addItem(self->spatialQuadMap, &position, (void*)blockQuadHandle);

    BlockInstance* blockInstanceDup = ememdup(blockInstance, 1, sizeof(BlockInstance));
    HashMap_addItem(self->spatialBlockInstanceMap, &position, (void*)blockInstanceDup);

    if (self->state == STATE_INITIALIZING) {
        self->nBlocksToShowInit = Math_max(self->nBlocksToShowInit, blockInstance->iTimeSlot + 1);
    }
}


static void ObjectView_onBlockInstanceRemoved(ObjectView* self, void* sender, BlockInstance* blockInstance) {
    (void)sender;
    Log_assert(self->state == STATE_IDLE, "Expected state to be %d, but was %d", STATE_IDLE, self->state);

    Vector2i position = {
        blockInstance->iTimeSlot,
        blockInstance->iTrack,
    };
    QuadHandle blockInstanceQuadHandle = (QuadHandle)HashMap_getItem(self->spatialQuadMap, &position);
    if (blockInstanceQuadHandle) {
        Grid_hideQuad(self->blocksGrid, blockInstanceQuadHandle);
    }
    HashMap_addItem(self->spatialQuadMap, &position, NULL);

    BlockInstance* blockInstanceDup = HashMap_getItem(self->spatialBlockInstanceMap, &position);
    if (blockInstanceDup) {
        sfree((void**)&blockInstanceDup);
        HashMap_addItem(self->spatialBlockInstanceMap, &position, NULL);
    }
}


static void ObjectView_onActiveBlockChanged(ObjectView* self, void* sender, Color* color) {
    (void)sender;
    self->blockColor = *color;
}


static void ObjectView_onBlockColorChanged(ObjectView* self, void* sender, Color* color) {
    (void)sender;
    self->blockColor = *color;
    ObjectView_refreshBlockGrid(self);
}


static void ObjectView_onSequencerStarted(ObjectView* self, void* sender, SequencerRequest* sequencerRequest) {
    (void)sender; (void)sequencerRequest;

    if (self->applicationState != APPLICATION_STATE_OBJECT_MODE) {
        return;
    }

    Grid_hideQuad(self->cursorGrid, self->cursorHandle);

    QuadHandle quadHandle = (QuadHandle)HashMap_getItem(self->spatialQuadMap, &self->cursorPosition);
    BlockInstance* blockInstance = HashMap_getItem(self->spatialBlockInstanceMap, &self->cursorPosition);
    if (quadHandle && blockInstance) {
        Color blockColor = blockInstance->color;
        Grid_updateQuadColor(self->blocksGrid, quadHandle, blockColor);
    }

    self->playbackCursorPosition = (Vector2i){0, 0};
    Grid_updateQuadPosition(self->cursorGrid, self->playbackCursorHandle, self->playbackCursorPosition);

    Color cursorInitialColor = lerpColor(CURSOR_COLOR, HIGHLIGHT_COLOR, PLAYBACK_CURSOR_HIGHLIGHT_STRENGTH);
    Grid_updateQuadColor(self->cursorGrid, self->playbackCursorHandle, cursorInitialColor);
    Grid_animateQuadColor(self->cursorGrid, self->playbackCursorHandle, CURSOR_COLOR, PLAYBACK_CURSOR_HIGHLIGHT_LERP_WEIGHT);

    self->state = STATE_PLAYING;

    self->sequencerRequest = ememdup(sequencerRequest, 1, sizeof(SequencerRequest));
    self->sequencerRequest->midiMessages = ememdup(sequencerRequest->midiMessages, sequencerRequest->nMidiMessages, sizeof(MidiMessage));
    self->iPlaybackMidiMessage = 0;
}


static void ObjectView_onSequencerStopped(ObjectView* self, void* sender, void* unused) {
    (void)sender; (void)unused;

    Grid_hideQuad(self->cursorGrid, self->playbackCursorHandle);
    Grid_unhideQuad(self->cursorGrid, self->cursorHandle);

    Grid_finalizeAllColorAnimations(self->blocksGrid);

    QuadHandle quadHandle = (QuadHandle)HashMap_getItem(self->spatialQuadMap, &self->cursorPosition);
    BlockInstance* blockInstance = HashMap_getItem(self->spatialBlockInstanceMap, &self->cursorPosition);
    if (quadHandle && blockInstance) {
        Color blockColor = blockInstance->color;
        Color blockHighlightColor = lerpColor(blockColor, HIGHLIGHT_COLOR, NOTE_HIGHLIGHT_STRENGTH);
        Grid_updateQuadColor(self->blocksGrid, quadHandle, blockHighlightColor);
    }

    if (self->sequencerRequest) {
        if (self->sequencerRequest->midiMessages) {
            sfree((void**)&self->sequencerRequest->midiMessages);
        }
        sfree((void**)&self->sequencerRequest);
    }
    self->iPlaybackMidiMessage = 0;

    self->state = STATE_IDLE;
}


static void ObjectView_onSequencerProgress(ObjectView* self, void* sender, float* progress) {
    (void)sender;

    if (self->applicationState != APPLICATION_STATE_OBJECT_MODE) {
        return;
    }

    Log_assert(self->state == STATE_PLAYING, "Unexpected sequencer progress update in state %d", self->state);

    // Try to sync audio and visuals better.
    float progressDelta = AUDIO_VISUAL_DELAY_COMPENSATION_SECONDS / self->sequencerRequest->timestampEnd;
    float delayAdjustedProgress = Math_maxf(Math_minf(*progress + progressDelta, 1.0f), 0.0f);

    int iTimeSlot = delayAdjustedProgress * (float)getGridSize().x;

    Grid_unhideQuad(self->cursorGrid, self->playbackCursorHandle);
    self->playbackCursorPosition = (Vector2i){iTimeSlot, 0};
    Grid_updateQuadPosition(self->cursorGrid, self->playbackCursorHandle, self->playbackCursorPosition);

    float sequencerTimestampSeconds = delayAdjustedProgress * self->sequencerRequest->timestampEnd;

    float lerpWeights[N_SYNTH_TRACKS] = {0};

    while (self->iPlaybackMidiMessage < self->sequencerRequest->nMidiMessages
            && self->sequencerRequest->midiMessages[self->iPlaybackMidiMessage].timestampSeconds < sequencerTimestampSeconds) {
        if (self->sequencerRequest->midiMessages[self->iPlaybackMidiMessage].type == MIDI_MESSAGE_TYPE_NOTEON) {
            int iChannel = self->sequencerRequest->midiMessages[self->iPlaybackMidiMessage].channel;
            Log_assert(iChannel > 0, "Synth channel index must be larger than 0, was %d", iChannel);
            int iTrack = iChannel - 1;

            int velocity = self->sequencerRequest->midiMessages[self->iPlaybackMidiMessage].velocity;
            float lerpWeight = (float)velocity / (float)MIDI_MESSAGE_VELOCITY_MAX;

            lerpWeights[iTrack] = Math_minf(lerpWeights[iTrack] + lerpWeight, 1.0);
        }

        self->iPlaybackMidiMessage++;
    }

    for (int iTrack = 0; iTrack < N_SYNTH_TRACKS; iTrack++) {
        if (lerpWeights[iTrack] > 0.0) {
            Vector2i position = {iTimeSlot, iTrack};
            QuadHandle quadHandle = (QuadHandle)HashMap_getItem(self->spatialQuadMap, &position);
            BlockInstance* blockInstance = HashMap_getItem(self->spatialBlockInstanceMap, &position);
            if (quadHandle && blockInstance) {
                Color blockColor = blockInstance->color;
                Color blockHighlightColor = lerpColor(blockColor, HIGHLIGHT_COLOR, Math_powf(lerpWeights[iTrack], 0.5));
                Grid_updateQuadColor(self->blocksGrid, quadHandle, blockHighlightColor);
                Grid_animateQuadColor(self->blocksGrid, quadHandle, blockColor, PLAYBACK_BLOCK_HIGHLIGHT_LERP_WEIGHT);
            }
        }
    }

    self->playbackCursorPositionPrev = self->playbackCursorPosition;
}


static void ObjectView_onApplicationStateChanged(ObjectView* self, void* sender, ApplicationState* applicationState) {
    (void)sender;

    self->applicationState = *applicationState;

    if (self->applicationState == APPLICATION_STATE_OBJECT_MODE) {
        ObjectView_unhide(self);
    } else {
        ObjectView_hide(self);
    }
}


static void ObjectView_onCursorPositionUpdated(ObjectView* self) {
    switch (self->state) {
        case STATE_IDLE:;
            Grid_unhideQuad(self->cursorGrid, self->cursorHandle);
            if (self->isBlockAddButtonPressed) {
                // Make sure that blocks are not skipped over when moving the cursor quickly
                Vector2i removalPosition = self->cursorPositionPrev;
                while(removalPosition.x != self->cursorPosition.x || removalPosition.y != self->cursorPosition.y) {
                    if (Math_abs(removalPosition.x - self->cursorPosition.x) > Math_abs(removalPosition.y - self->cursorPosition.y)) {
                        removalPosition.x = Math_max(removalPosition.x - 1, Math_min(removalPosition.x + 1, self->cursorPosition.x));
                    } else {
                        removalPosition.y = Math_max(removalPosition.y - 1, Math_min(removalPosition.y + 1, self->cursorPosition.y));
                    }
                    ObjectView_addBlockInstanceAtPosition(self, removalPosition);
                }
            }
            if (self->isBlockRemovalButtonPressed) {
                // Make sure that blocks are not skipped over when moving the cursor quickly
                Vector2i removalPosition = self->cursorPositionPrev;
                while(removalPosition.x != self->cursorPosition.x || removalPosition.y != self->cursorPosition.y) {
                    if (Math_abs(removalPosition.x - self->cursorPosition.x) > Math_abs(removalPosition.y - self->cursorPosition.y)) {
                        removalPosition.x = Math_max(removalPosition.x - 1, Math_min(removalPosition.x + 1, self->cursorPosition.x));
                    } else {
                        removalPosition.y = Math_max(removalPosition.y - 1, Math_min(removalPosition.y + 1, self->cursorPosition.y));
                    }
                    ObjectView_removeBlockInstanceAtPosition(self, removalPosition);
                }
            }

            QuadHandle quadHandle = (QuadHandle)HashMap_getItem(self->spatialQuadMap, &self->cursorPosition);
            BlockInstance* blockInstance = HashMap_getItem(self->spatialBlockInstanceMap, &self->cursorPosition);
            QuadHandle quadHandlePrev = (QuadHandle)HashMap_getItem(self->spatialQuadMap, &self->cursorPositionPrev);
            BlockInstance* blockInstancePrev = HashMap_getItem(self->spatialBlockInstanceMap, &self->cursorPositionPrev);
            if (quadHandle != quadHandlePrev) {
                if (quadHandle && blockInstance) {
                    Color blockColor = blockInstance->color;
                    Color blockHighlightColor = lerpColor(blockColor, HIGHLIGHT_COLOR, NOTE_HIGHLIGHT_STRENGTH);
                    Grid_updateQuadColor(self->blocksGrid, quadHandle, blockHighlightColor);
                }
                if (quadHandlePrev && blockInstancePrev) {
                    Color blockColorPrev = blockInstancePrev->color;
                    Grid_updateQuadColor(self->blocksGrid, quadHandlePrev, blockColorPrev);
                }
            }
            break;
        default:;
    }
}


static void ObjectView_addBlockInstanceAtCursor(ObjectView* self) {
    ObjectView_addBlockInstanceAtPosition(self, self->cursorPosition);
}


static void ObjectView_addBlockInstanceAtPosition(ObjectView* self, Vector2i position) {
    Score_addBlockInstance(self->score, position.y, position.x);
}


static void ObjectView_removeBlockInstanceAtCursor(ObjectView* self) {
    ObjectView_removeBlockInstanceAtPosition(self, self->cursorPosition);
}


static void ObjectView_removeBlockInstanceAtPosition(ObjectView* self, Vector2i position) {
    Score_removeBlockInstance(self->score, position.y, position.x);
}


static void ObjectView_hide(ObjectView* self) {
    Score_stopPlaying(self->score);

    Grid_hide(self->gridlinesGrid);
    Grid_hide(self->cursorGrid);
    Grid_hide(self->blocksGrid);
}


static void ObjectView_unhide(ObjectView* self) {
    Grid_unhide(self->gridlinesGrid);
    Grid_unhide(self->cursorGrid);
    Grid_unhide(self->blocksGrid);
}


static void ObjectView_refreshBlockGrid(ObjectView* self) {
    Grid_free(&self->blocksGrid);
    self->blocksGrid = Grid_new(getGridSize(), (Vector2){BLOCKS_GRID_CELL_SPACING, BLOCKS_GRID_CELL_SPACING});
    Grid_zoomTo(self->blocksGrid, (Vector2i){self->scrollX, 0}, (Vector2i){self->nBlocksToShow, getGridSize().y});
    Grid_hide(self->blocksGrid);

    for (Vector2i* position = HashMap_iterateInit(self->spatialBlockInstanceMap); position; position = HashMap_iterateNext(self->spatialBlockInstanceMap, position)) {
        BlockInstance* blockInstance = HashMap_getItem(self->spatialBlockInstanceMap, position);
        if (blockInstance) {
            sfree((void**)&blockInstance);
        }
    }

    for (int x = 0; x < getGridSize().x; x++) {
        for (int y = 0; y < getGridSize().y; y++) {
            Vector2i position = {x, y};
            HashMap_addItem(self->spatialQuadMap, &position, NULL);
            HashMap_addItem(self->spatialBlockInstanceMap, &position, NULL);
        }
    }

    self->state = STATE_INITIALIZING;
    Score_requestBlockInstances(self->score);

    self->state = STATE_IDLE;

    if (self->applicationState == APPLICATION_STATE_OBJECT_MODE) {
        Grid_unhide(self->blocksGrid);
    }
}


static Vector2i getGridSize(void) {
    return (Vector2i){SCORE_LENGTH_MAX, N_SYNTH_TRACKS};
}
