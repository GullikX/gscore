const char* const VERTEX_SHADER_SOURCE =
    "#version 120\n"
    "attribute vec4 color;"
    "void main() {"
    "   gl_FrontColor = color;"
    "   gl_Position = gl_Vertex;"
    "}";

const char* const FRAGMENT_SHADER_SOURCE =
    "#version 120\n"
    "void main() {"
    "    gl_FragColor = gl_Color;"
    "}";

const char* const WINDOW_TITLE = "GScore";

enum {
    WINDOW_WIDTH = 640,
    WINDOW_HEIGHT = 640,
    RENDERER_MAX_QUADS = 1000,
    RENDERER_MAX_VERTICES = 4*RENDERER_MAX_QUADS,
};

const Vector4 COLOR_1 = {0.0f, 0.5f, 1.0f, 1.0f};
const Vector4 COLOR_2 = {1.0f, 0.5f, 0.0f, 1.0f};
