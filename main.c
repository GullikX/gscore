int main() {
    Synth_getInstance();
    while(Renderer_running()) {
        Canvas_draw();
        Renderer_updateScreen();
    }
}
