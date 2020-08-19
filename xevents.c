XEvents* XEvents_getInstance() {
    static XEvents* self = NULL;
    if (self) return self;

    self = ecalloc(1, sizeof(*self));

    GLFWwindow* glfwWindow = Renderer_getInstance()->window;
    self->x11Display = glfwGetX11Display();
    self->x11Window = glfwGetX11Window(glfwWindow);
    printf("x11Window id: %lu\n", self->x11Window);
    for (int i = 0; i < ATOM_COUNT; i++) {
        self->atoms[i] = XInternAtom(self->x11Display, ATOM_NAMES[i], false);
    }

    return self;
}

void XEvents_processXEvents() {
    XEvents* self = XEvents_getInstance();

    for (int i = 0; i < ATOM_COUNT; i++) {
        unsigned char* propertyValue = NULL;
        Atom atomDummy;
        int intDummy;
        unsigned long longDummy;

        int result = XGetWindowProperty(
            self->x11Display,
            self->x11Window,
            self->atoms[i],
            0L,
            sizeof(Atom),
            true,
            XA_STRING,
            &atomDummy,
            &intDummy,
            &longDummy,
            &longDummy,
            &propertyValue
        );

        if (result == Success && propertyValue) {
            printf("Received: %s=%s\n", ATOM_NAMES[i], propertyValue);
        }
        XFree(propertyValue);
    }
}
