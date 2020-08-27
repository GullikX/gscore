int main(void) {
    Synth_getInstance();
    while(Renderer_running()) {
        Player_update();
        Canvas_draw();
        Player_drawCursor();
        Renderer_updateScreen();
    }
}
