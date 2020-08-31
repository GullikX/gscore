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
    /* TODO */
    self->score = score;
    self->playing = true;
    self->startTime = glfwGetTime();
}


void ScorePlayer_stop(ScorePlayer* self) {
    /* TODO */
    self->score = NULL;
    self->playing = false;
    self->startTime = 0.0f;
}


bool ScorePlayer_playing(ScorePlayer* self) {
    return self->playing;
}


void ScorePlayer_update(ScorePlayer* self) {
    (void)self;
    /* TODO */
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
