void die(const char* const message) {
    fprintf(stderr, "Error: %s\n", message);
    exit(EXIT_FAILURE);
}


void* ecalloc(size_t nItems, size_t itemSize) {
    void* pointer = calloc(nItems, itemSize);
    if (!pointer) {
        die("Failed to allocate memory");
    }
    return pointer;
}


void spawnSetXProp(int atomId) {
    char windowId[64];
    snprintf(windowId, 64, "%lu", XEvents_getInstance()->x11Window);
    const char* cmd[] = CMD_SET_XPROP(ATOM_PROMPTS[atomId], ATOM_NAMES[atomId], windowId);
    spawn(cmd);
}


void spawn(const char* cmd[]) {
    if (!fork()) {
        setsid();
        execvp(((char**)cmd)[0], (char**)cmd);
    }
}
