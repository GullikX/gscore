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

ScorePlayer* ScorePlayer_new(Score* score) {
    ScorePlayer* self = ecalloc(1, sizeof(*self));

    self->score = score;
    self->playing = false;
    self->startTime = 0.0f;

    return self;
}


ScorePlayer* ScorePlayer_free(ScorePlayer* self) {
    ScorePlayer_stop(self);
    free(self);
    return NULL;
}


void ScorePlayer_playScore(ScorePlayer* self) {
    self->playing = true;
    self->startTime = glfwGetTime();
    float blockTime = (float)(BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE) / (float)self->score->tempo;

    for (int iTrack = 0; iTrack < N_TRACKS; iTrack++) {
        Synth_setProgramById(Application_getInstance()->synth, iTrack + 1, self->score->tracks[iTrack].program);

        for (int iBlock = 0; iBlock < SCORE_LENGTH; iBlock++) {
            Block* block = self->score->tracks[iTrack].blocks[iBlock];
            if (!block) continue;
            for (MidiMessage* midiMessage = block->midiMessageRoot; midiMessage; midiMessage = midiMessage->next) {
                if (midiMessage->time < 0) continue;
                int channel = iTrack + 1;
                float velocity = midiMessage->velocity * self->score->tracks[iTrack].velocity * self->score->tracks[iTrack].blockVelocities[iBlock];
                float time = midiMessage->time * blockTime + blockTime * iBlock;

                switch (midiMessage->type) {
                    case FLUID_SEQ_NOTEON:
                        Synth_sendNoteOn(Application_getInstance()->synth, channel, midiMessage->pitch, velocity, time);
                        break;
                    case FLUID_SEQ_NOTEOFF:
                        Synth_sendNoteOff(Application_getInstance()->synth, channel, midiMessage->pitch, time);
                        break;
                }
            }
        }
    }
}


void ScorePlayer_stop(ScorePlayer* self) {
    /* TODO: free memory */
    self->playing = false;
    self->startTime = 0.0f;
    Synth_noteOffAll(Application_getInstance()->synth);
}


bool ScorePlayer_playing(ScorePlayer* self) {
    return self->playing;
}


void ScorePlayer_update(ScorePlayer* self) {
    if (!ScorePlayer_playing(self)) return;

    float time = glfwGetTime() - self->startTime;
    float totalTime = SCORE_LENGTH * BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE / self->score->tempo;

    if (time < totalTime) {
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


void ScorePlayer_setProgram(ScorePlayer* self, int program) {
    int iTrack = Application_getInstance()->objectView->cursor.iRow;
    self->score->tracks[iTrack].program = program;
}
