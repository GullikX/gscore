Player* Player_getInstance(void) {
    static Player* self = NULL;
    if (self) return self;

    self = ecalloc(1, sizeof(*self));
    self->playing = false;
    self->startTime = 0;
    Player_setTempoBpm(TEMPO_BPM);

    return self;
}


void Player_setTempoBpm(int tempoBpm) {
    Player* self = Player_getInstance();
    self->tempoBpm = tempoBpm;
    snprintf(self->tempoBpmString, 64, "%d", tempoBpm);
}


void Player_toggle(void) {
    if (Player_playing()) {
        Player_stop();
    }
    else {
        Player_start();
    }
}


bool Player_playing(void) {
    return Player_getInstance()->playing;
}


void Player_start(void) {
    Player* self = Player_getInstance();
    self->playing = true;
    self->startTime = glfwGetTime();
    printf("Start playing at time: %f\n", self->startTime);
}


void Player_stop(void) {
    puts("Stop playing");
    Player* self = Player_getInstance();
    self->playing = false;
    Canvas_resetPlayerCursorPosition();
}


void Player_update(void) {
    Player* self = Player_getInstance();
    if (!self->playing) return;

    double time = glfwGetTime() - self->startTime;
    double totalTime = BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE / self->tempoBpm;

    int viewportWidth = Renderer_getInstance()->viewportWidth;
    float cursorX = (float)viewportWidth * time / totalTime;

    if (cursorX < viewportWidth) {
        Canvas_updatePlayerCursorPosition(cursorX);
    } else {
        Player_stop();
    }
}


char* Player_getTempoBpmString(void) {
    Player* self = Player_getInstance();
    return self->tempoBpmString;
}
