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

BlockPlayer* BlockPlayer_new(void) {
    BlockPlayer* self = ecalloc(1, sizeof(*self));
    self->channel = 0;  /* TODO */
    self->playing = false;
    self->repeat = false;
    self->startTime = 0;
    BlockPlayer_setTempoBpm(self, TEMPO_BPM);

    return self;
}


BlockPlayer* BlockPlayer_free(BlockPlayer* self) {
    BlockPlayer_stop(self);
    free(self);
    return NULL;
}


void BlockPlayer_setTempoBpm(BlockPlayer* self, int tempoBpm) {
    self->tempoBpm = tempoBpm;
    snprintf(self->tempoBpmString, 64, "%d", tempoBpm);
}


bool BlockPlayer_playing(BlockPlayer* self) {
    return self->playing;
}


void BlockPlayer_playBlock(BlockPlayer* self, Block* block, float startPosition, bool repeat) {
    self->midiMessage = block->midiMessageRoot;
    self->playing = true;
    self->startPosition = startPosition;
    self->repeat = repeat;
    self->startTime = glfwGetTime();

    while (self->midiMessage && self->midiMessage->time < startPosition) {
        self->midiMessage = self->midiMessage->next;
    }
    printf("Start playing at time: %f, position %f, repeat %s\n", self->startTime, startPosition, repeat ? "true" : "false");
}


void BlockPlayer_stop(BlockPlayer* self) {
    puts("Player: Stop playing");
    self->playing = false;
    self->repeat = false;
    Synth_noteOffAll(Application_getInstance()->synth);
}


void BlockPlayer_update(BlockPlayer* self) {
    if (!BlockPlayer_playing(self)) return;

    float time = glfwGetTime() - self->startTime;
    float totalTime = BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE / self->tempoBpm;
    float progress = self->startPosition + time / totalTime;

    if (progress < 1.0f) {
        while (self->midiMessage && self->midiMessage->time < progress) {
            Synth_processMessage(Application_getInstance()->synth, self->channel, self->midiMessage);
            self->midiMessage = self->midiMessage->next;
        }
    } else {
        if (false /*self->repeat*/) {
            //Player_start(0.0f, true);
        }
        else {
            BlockPlayer_stop(self);
        }
    }
}


void BlockPlayer_drawCursor(BlockPlayer* self) {
    if (!BlockPlayer_playing(self)) return;

    float time = glfwGetTime() - self->startTime;
    float totalTime = BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE / self->tempoBpm;
    float progress = self->startPosition + time / totalTime;
    float cursorX = -1.0f + 2.0f * progress;

    float x1 = cursorX;
    float x2 = cursorX + PLAYER_CURSOR_WIDTH;
    float y1 = -1.0f;
    float y2 = 1.0f;

    Renderer_drawQuad(Application_getInstance()->renderer, x1, x2, y1, y2, COLOR_CURSOR);
}


char* BlockPlayer_getTempoBpmString(void) {
    BlockPlayer* self = Application_getInstance()->editView->player;
    return self->tempoBpmString;
}
