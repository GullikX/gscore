int main() {
    while(Renderer_running()) {
        Input_processInput();
        Renderer_updateScreen();
    }
}
