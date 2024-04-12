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

#pragma once

#include <stdbool.h>

typedef struct Score Score;

Score* Score_new(const char* const filename);
void Score_free(Score** pself);
void Score_addNote(Score* self, int pitch, float time, float duration, float velocity);
void Score_removeNotes(Score* self, int pitch, float timeStart, float timeEnd);
void Score_saveToFile(Score* self);
void Score_playCurrentBlockDef(Score* self, int iChannel, float startTimeFraction, bool ignoreNoteOff);
void Score_playEntireScore(Score* self, int iTimeSlotStart);
void Score_requestCurrentBlockDefNotes(Score* self);
void Score_requestPrevBlockDefNotes(Score* self);
void Score_requestCurrentBlockDefColor(Score* self);
void Score_requestKeySignature(Score* self);
void Score_requestBlockInstances(Score* self);
void Score_requestSynthPrograms(Score* self);
void Score_stopPlaying(Score* self);
void Score_changeKeySignature(Score* self);
void Score_changeActiveBlockDef(Score* self);
void Score_toggleActiveBlockDef(Score* self);
void Score_pickBlockDef(Score* self, int iTrack, int iTimeSlot);
void Score_changeCurrentBlockDefColor(Score* self);
void Score_renameCurrentBlockDef(Score* self);
void Score_setTempoBpm(Score* self);
void Score_changeTrackVelocity(Score* self, int iTrack);
void Score_addBlockInstance(Score* self, int iTrack, int iTimeSlot);
void Score_removeBlockInstance(Score* self, int iTrack, int iTimeSlot);
void Score_toggleIgnoreNoteOff(Score* self, int iTrack);
