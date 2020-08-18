int main() {
    while(Renderer_running()) {
        Canvas_draw();
        Renderer_updateScreen();
    }
}
