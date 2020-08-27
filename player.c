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


void Player_drawCursor(void) {
    Player* self = Player_getInstance();
    if (!self->playing) return;

    double time = glfwGetTime() - self->startTime;
    double totalTime = BLOCK_MEASURES * BEATS_PER_MEASURE * SECONDS_PER_MINUTE / self->tempoBpm;

    float cursorX = -1.0f + 2.0f * (float)time / totalTime;

    float x1 = cursorX;
    float x2 = cursorX + PLAYER_CURSOR_WIDTH;
    float y1 = -1.0f;
    float y2 = 1.0f;

    Vector2 positions[4];
    positions[0].x = x1; positions[0].y = y1;
    positions[1].x = x1; positions[1].y = y2;
    positions[2].x = x2; positions[2].y = y2;
    positions[3].x = x2; positions[3].y = y1;

    Quad quad;
    for (int i = 0; i < 4; i++) {
        quad.vertices[i].position = positions[i];
        quad.vertices[i].color = COLOR_CURSOR;
    }

    Renderer_enqueueDraw(&quad);
}


char* Player_getTempoBpmString(void) {
    Player* self = Player_getInstance();
    return self->tempoBpmString;
}
