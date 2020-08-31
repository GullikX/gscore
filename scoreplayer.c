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

ScorePlayer* ScorePlayer_new(void) {
    ScorePlayer* self = ecalloc(1, sizeof(*self));

    self->score = NULL;
    self->playing = false;
    self->startTime = 0.0f;

    return self;
}


ScorePlayer* ScorePlayer_free(ScorePlayer* self) {
    ScorePlayer_stop(self);
    free(self);
    return NULL;
}


void ScorePlayer_playScore(ScorePlayer* self, Score* score) {
    self->score = score;
    self->playing = true;
    self->startTime = glfwGetTime();
    float blockTime = (float)(BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE) / (float)score->tempo;

    for (int iTrack = 0; iTrack < N_TRACKS; iTrack++) {
        self->midiMessages[iTrack] = ecalloc(1, sizeof(MidiMessage));
        self->midiMessages[iTrack]->type = FLUID_SEQ_NOTE;
        self->midiMessages[iTrack]->time = -1.0f;
        self->midiMessages[iTrack]->pitch = -1;
        self->midiMessages[iTrack]->velocity = -1;
        self->midiMessages[iTrack]->next = NULL;
        self->midiMessages[iTrack]->prev = NULL;
        MidiMessage* midiMessageSelf = self->midiMessages[iTrack];

        for (int iBlock = 0; iBlock < SCORE_LENGTH; iBlock++) {
            Block* block = score->tracks[iTrack].blocks[iBlock];
            if (!block) continue;
            for (MidiMessage* midiMessage = block->midiMessageRoot; midiMessage; midiMessage = midiMessage->next) {
                if (midiMessage->time < 0) continue;
                midiMessageSelf->next = ecalloc(1, sizeof(MidiMessage));
                midiMessageSelf->next->type = midiMessage->type;
                midiMessageSelf->next->time = midiMessage->time * blockTime + blockTime * iBlock;
                midiMessageSelf->next->pitch = midiMessage->pitch;
                midiMessageSelf->next->velocity = midiMessage->velocity;
                midiMessageSelf->next->next = NULL;
                midiMessageSelf->next->prev = midiMessageSelf;
                midiMessageSelf = midiMessageSelf->next;
            }
        }
        Synth_setProgramById(Application_getInstance()->synth, iTrack + 1, iTrack * 4);  /* TODO */
    }
}


void ScorePlayer_stop(ScorePlayer* self) {
    /* TODO: free memory */
    self->score = NULL;
    self->playing = false;
    self->startTime = 0.0f;
}


bool ScorePlayer_playing(ScorePlayer* self) {
    return self->playing;
}


void ScorePlayer_update(ScorePlayer* self) {
    if (!ScorePlayer_playing(self)) return;

    float time = glfwGetTime() - self->startTime;
    float totalTime = SCORE_LENGTH * BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE / self->score->tempo;

    if (time < totalTime) {
        for (int iTrack = 0; iTrack < N_TRACKS; iTrack++)
        while (self->midiMessages[iTrack] && self->midiMessages[iTrack]->time < time) {
            Synth_processMessage(Application_getInstance()->synth, iTrack + 1, self->midiMessages[iTrack]);
            self->midiMessages[iTrack] = self->midiMessages[iTrack]->next;
        }
    }
    else {
        ScorePlayer_stop(self);
    }
}


void ScorePlayer_drawCursor(ScorePlayer* self) {
    if (!ScorePlayer_playing(self)) return;
    float time = glfwGetTime() - self->startTime;
    int tempoBpm = self->score->tempo;
    float totalTime = SCORE_LENGTH * BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE / tempoBpm;
    float progress = time / totalTime;
    float cursorX = -1.0f + 2.0f * progress;

    float x1 = cursorX;
    float x2 = cursorX + PLAYER_CURSOR_WIDTH;
    float y1 = -1.0f;
    float y2 = 1.0f;

    Renderer_drawQuad(Application_getInstance()->renderer, x1, x2, y1, y2, COLOR_CURSOR);
}
