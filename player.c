Player* Player_getInstance() {
    static Player* self = NULL;
    if (self) return self;

    self = ecalloc(1, sizeof(*self));
    self->playing = false;
    self->startTime = 0;
    self->tempoBpm = TEMPO_BPM;

    return self;
}


void Player_setTempoBpm(int tempoBpm) {
    Player* self = Player_getInstance();
    self->tempoBpm = tempoBpm;
}


void Player_toggle() {
    if (Player_playing()) {
        Player_stop();
    }
    else {
        Player_start();
    }
}


bool Player_playing() {
    return Player_getInstance()->playing;
}


void Player_start() {
    Player* self = Player_getInstance();
    self->playing = true;
    self->startTime = glfwGetTime();
    printf("Start playing at time: %f\n", self->startTime);
}


void Player_stop() {
    puts("Stop playing");
    Player* self = Player_getInstance();
    self->playing = false;
    Canvas_resetPlayerCursorPosition();
}


void Player_update() {
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
