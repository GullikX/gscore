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


void Player_start(float startPosition, bool repeat) {
    Player* self = Player_getInstance();
    self->playing = true;
    self->startPosition = startPosition;
    self->repeat = repeat;
    self->startTime = glfwGetTime();
    printf("Start playing at time: %f, position %f, repeat %s\n", self->startTime, startPosition, repeat ? "true" : "false");
}


void Player_stop(void) {
    puts("Stop playing");
    Player* self = Player_getInstance();
    self->playing = false;
    self->repeat = false;
    Canvas_resetPlayerCursorPosition();
}


void Player_update(void) {
    Player* self = Player_getInstance();
    if (!self->playing) return;

    float time = glfwGetTime() - self->startTime;
    float totalTime = BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE / self->tempoBpm;
    float progress = self->startPosition + time / totalTime;

    if (progress < 1.0f) {
        Canvas_updatePlayerCursorPosition(progress);
    } else {
        if (self->repeat) {
            Player_start(0.0f, true);
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
