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

ScorePlayer* ScorePlayer_getInstance(void) {
    static ScorePlayer* self = NULL;
    if (self) return self;

    self = ecalloc(1, sizeof(*self));
    self->playing = false;

    return self;
}


bool ScorePlayer_playing(void) {
    return ScorePlayer_getInstance()->playing;
}


void ScorePlayer_playScore(Score* score) {
    ScorePlayer* self = ScorePlayer_getInstance();
    self->score = score;
    self->playing = true;
    self->startTime = glfwGetTime();
    self->iBlock = -1;

    printf("Start playing at time: %f\n", self->startTime);
}


void ScorePlayer_stop(void) {
    puts("ScorePlayer: Stop playing");
    ScorePlayer* self = ScorePlayer_getInstance();
    self->playing = false;
    Synth_noteOffAll();
}


void ScorePlayer_update(void) {
    if (!ScorePlayer_playing()) return;
    ScorePlayer* self = ScorePlayer_getInstance();

    float time = glfwGetTime() - self->startTime;
    int tempoBpm = self->score->tempo;
    float totalTime = SCORE_LENGTH * BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE / tempoBpm;
    float progress = time / totalTime;

    if (progress < 1.0f) {
        Player_update();
        int iBlock = SCORE_LENGTH * progress;
        if (iBlock > self->iBlock) {
            Player_stop();
            Block* block = self->score->tracks[0].blocks[iBlock];
            if (block) {
                Player_setTempoBpm(self->score->tempo);
                Player_playBlock(block, 0.0f, false);
            }
            self->iBlock = iBlock;
        }
    } else {
        ScorePlayer_stop();
    }
}


void ScorePlayer_drawCursor(void) {
    ScorePlayer* self = ScorePlayer_getInstance();
    if (!self->playing) return;

    float time = glfwGetTime() - self->startTime;
    int tempoBpm = self->score->tempo;
    float totalTime = SCORE_LENGTH * BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE / tempoBpm;
    float progress = time / totalTime;
    float cursorX = -1.0f + 2.0f * progress;

    float x1 = cursorX;
    float x2 = cursorX + PLAYER_CURSOR_WIDTH;
    float y1 = -1.0f;
    float y2 = 1.0f;

    Renderer_drawQuad(x1, x2, y1, y2, COLOR_CURSOR);
}
