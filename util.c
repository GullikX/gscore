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
    int bufferLength = strlen(cmdQuery) + strlen(ATOM_PROMPTS[atomId]) + strlen(ATOM_NAMES[atomId]) + 64;
    char* cmd = ecalloc(bufferLength, sizeof(char));
    snprintf(cmd, bufferLength, cmdQuery, XEvents_getInstance()->x11Window, ATOM_PROMPTS[atomId], ATOM_NAMES[atomId]);
    const char* const pipeData = ATOM_FUNCTIONS[atomId]();
    spawn(cmd, pipeData);
}


void spawn(const char* const cmd, const char* const pipeData) {
    FILE* pipe = popen(cmd, "w");
    if (!pipe) {
        die("Failed to run popen");
    }
    fprintf(pipe, "%s", pipeData);
    pclose(pipe);
}
