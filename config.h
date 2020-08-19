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

const char* const AUDIO_DRIVER = "alsa";
const char* const ALSA_DEVICE = "default:CARD=S3";
const char* const SOUNDFONT = "soundfont.sf2";

enum {
    WINDOW_WIDTH = 640,
    WINDOW_HEIGHT = 640,
    RENDERER_MAX_QUADS = 1000,
    RENDERER_MAX_VERTICES = 4*RENDERER_MAX_QUADS,
    CANVAS_MAX_NOTES = RENDERER_MAX_QUADS,
    BEATS_PER_MEASURE = 4,
    SECONDS_PER_MINUTE = 60,
    TEMPO_BPM = 100,
    BLOCK_MEASURES = 4,
    MEASURE_RESOLUTION = 16,
    OCTAVES = 5,
    NOTES_IN_OCTAVE = 12,
    MOUSE_POINTER_HIDDEN=true,
    SYNTH_AUDIO_PERIODS=2,
    SYNTH_AUDIO_PERIOD_SIZE=64,
    SYNTH_MIDI_CHANNELS=16,
    SYNTH_PROGRAM=0,
    SYNTH_ENABLE_REVERB=false,
    SYNTH_ENABLE_CHORUS=false,
};

enum {
    ATOM_BPM,
    ATOM_SYNTH_PROGRAM,
    ATOM_COUNT,
};

const char* ATOM_NAMES[] =  {
    "_GSCORE_BPM",
    "_GSCORE_SYNTH_PROGRAM",
};

const float CURSOR_SIZE_OFFSET = -0.005f;

const Vector4 COLOR_NOTES = {0.5411f, 0.7765f, 0.9490f, 1.0f};
const Vector4 COLOR_2 = {0.9412f, 0.7765f, 0.4549f, 1.0f};
const Vector4 COLOR_BACKGROUND = {0.1490f, 0.1961f, 0.2196f, 1.0f};
const Vector4 COLOR_GRIDLINES = {0.1294f, 0.1764, 0.1960, 1.0f};
const Vector4 COLOR_CURSOR = {0.72f, 0.72f, 0.72f, 1.0f};
