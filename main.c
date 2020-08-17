int main() {
    Renderer* renderer = Renderer_new();
    while(Renderer_running(renderer)) {
        Renderer_update(renderer);
    }
    renderer = Renderer_free(renderer);
}
