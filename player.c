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

Player* Player_getInstance(void) {
    static Player* self = NULL;
    if (self) return self;

    self = ecalloc(1, sizeof(*self));
    self->playing = false;
    self->repeat = false;
    self->startTime = 0;
    Player_setTempoBpm(TEMPO_BPM);

    return self;
}


void Player_setTempoBpm(int tempoBpm) {
    Player* self = Player_getInstance();
    self->tempoBpm = tempoBpm;
    snprintf(self->tempoBpmString, 64, "%d", tempoBpm);
}


bool Player_playing(void) {
    return Player_getInstance()->playing;
}


void Player_start(MidiMessage* midiMessageRoot, float startPosition, bool repeat) {
    Player* self = Player_getInstance();
    self->midiMessage = midiMessageRoot;
    self->playing = true;
    self->startPosition = startPosition;
    self->repeat = repeat;
    self->startTime = glfwGetTime();

    while (self->midiMessage && self->midiMessage->time < startPosition) {
        self->midiMessage = self->midiMessage->next;
    }
    printf("Start playing at time: %f, position %f, repeat %s\n", self->startTime, startPosition, repeat ? "true" : "false");
}


void Player_stop(void) {
    puts("Stop playing");
    Player* self = Player_getInstance();
    self->playing = false;
    self->repeat = false;
    Synth_noteOffAll();
}


void Player_update(void) {
    if (!Player_playing()) return;
    Player* self = Player_getInstance();

    float time = glfwGetTime() - self->startTime;
    float totalTime = BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE / self->tempoBpm;
    float progress = self->startPosition + time / totalTime;

    if (progress < 1.0f) {
        while (self->midiMessage && self->midiMessage->time < progress) {
            Synth_processMessage(self->midiMessage);
            self->midiMessage = self->midiMessage->next;
        }
    } else {
        if (false /*self->repeat*/) {
            //Player_start(0.0f, true);
        }
        else {
            Player_stop();
        }
    }
}


void Player_drawCursor(void) {
    Player* self = Player_getInstance();
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


char* Player_getTempoBpmString(void) {
    Player* self = Player_getInstance();
    return self->tempoBpmString;
}
