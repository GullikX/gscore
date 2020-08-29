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
const char* const ALSA_DEVICE = "default";
const char* const SOUNDFONT = "soundfont.sf2";

enum {
    WINDOW_WIDTH = 640,
    WINDOW_HEIGHT = 640,
    RENDERER_MAX_QUADS = 1000,
    RENDERER_MAX_VERTICES = 4*RENDERER_MAX_QUADS,
    MAX_BLOCKS = 64,
    N_TRACKS = 4,
    SCORE_LENGTH = 16,
    BEATS_PER_MEASURE = 4,
    SECONDS_PER_MINUTE = 60,
    TEMPO_BPM = 100,
    TEMPO_BPM_MAX = 1024,
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

const float CURSOR_SIZE_OFFSET = -0.005f;
const float NOTE_SIZE_OFFSET = -0.002f;
const float PLAYER_CURSOR_WIDTH = 0.005f;
const float MAX_TRACK_HEIGHT = 0.025f;
const float SYNTH_GAIN = 1.0f;

const Vector4 COLOR_NOTES = {0.5411f, 0.7765f, 0.9490f, 1.0f};
const Vector4 COLOR_2 = {0.9412f, 0.7765f, 0.4549f, 1.0f};
const Vector4 COLOR_BACKGROUND = {0.1490f, 0.1961f, 0.2196f, 1.0f};
const Vector4 COLOR_GRIDLINES = {0.1294f, 0.1764, 0.1960, 1.0f};
const Vector4 COLOR_CURSOR = {0.72f, 0.72f, 0.72f, 1.0f};

enum {
    ATOM_BPM,
    ATOM_SYNTH_PROGRAM,
    ATOM_COUNT,
};

const char* ATOM_NAMES[] =  {
    "_GSCORE_BPM",
    "_GSCORE_SYNTH_PROGRAM",
};

const char* ATOM_PROMPTS[] =  {
    "Set tempo (BPM):",
    "Set instrument:",
};

char* (*ATOM_FUNCTIONS[ATOM_COUNT])(void) = {
    Player_getTempoBpmString,
    Synth_getInstrumentListString,
};

const char* const cmdQuery =  /* sprintf(cmdQueryFull, cmdQuery, windowId, prompt, atomName) */
    "export WINDOWID=\"%lu\";"
    "export PROMPT=\"%s\";"
    "export ATOM_NAME=\"%s\";"
    "dmenu -b -i -p \"$PROMPT\" -w \"$WINDOWID\""
    "| tr '\\n' '\\0' "
    "| xargs -r0 xprop -id \"$WINDOWID\" -f \"$ATOM_NAME\" 8s -set \"$ATOM_NAME\"";
