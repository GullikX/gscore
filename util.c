void die(const char* const message) {
    printf("Error: %s\n", message);
    exit(EXIT_FAILURE);
}


void* ecalloc(size_t nItems, size_t itemSize) {
    void* pointer = calloc(nItems, itemSize);
    if (!pointer) {
        die("Failed to allocate memory");
    }
    return pointer;
}
