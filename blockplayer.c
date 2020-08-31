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

BlockPlayer* BlockPlayer_getInstance(void) {
    static BlockPlayer* self = NULL;
    if (self) return self;

    self = ecalloc(1, sizeof(*self));
    self->channel = 0;  /* TODO */
    self->playing = false;
    self->repeat = false;
    self->startTime = 0;
    BlockPlayer_setTempoBpm(TEMPO_BPM);

    return self;
}


void BlockPlayer_setTempoBpm(int tempoBpm) {
    BlockPlayer* self = BlockPlayer_getInstance();
    self->tempoBpm = tempoBpm;
    snprintf(self->tempoBpmString, 64, "%d", tempoBpm);
}


bool BlockPlayer_playing(void) {
    return BlockPlayer_getInstance()->playing;
}


void BlockPlayer_playBlock(Block* block, float startPosition, bool repeat) {
    BlockPlayer* self = BlockPlayer_getInstance();
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


void BlockPlayer_stop(void) {
    puts("Player: Stop playing");
    BlockPlayer* self = BlockPlayer_getInstance();
    self->playing = false;
    self->repeat = false;
    Synth_noteOffAll(Application_getInstance()->synth);
}


void BlockPlayer_update(void) {
    if (!BlockPlayer_playing()) return;
    BlockPlayer* self = BlockPlayer_getInstance();

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
            BlockPlayer_stop();
        }
    }
}


void BlockPlayer_drawCursor(void) {
    BlockPlayer* self = BlockPlayer_getInstance();
    if (!self->playing) return;

    float time = glfwGetTime() - self->startTime;
    float totalTime = BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE / self->tempoBpm;
    float progress = self->startPosition + time / totalTime;
    float cursorX = -1.0f + 2.0f * progress;

    float x1 = cursorX;
    float x2 = cursorX + PLAYER_CURSOR_WIDTH;
    float y1 = -1.0f;
    float y2 = 1.0f;

    Renderer_drawQuad(x1, x2, y1, y2, COLOR_CURSOR);
}


char* BlockPlayer_getTempoBpmString(void) {
    BlockPlayer* self = BlockPlayer_getInstance();
    return self->tempoBpmString;
}
