int main() {
    Synth_getInstance();
    while(Renderer_running()) {
        Player_update();
        Canvas_draw();
        Renderer_updateScreen();
    }
}
