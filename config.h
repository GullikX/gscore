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

const char* const BLOCK_NAME_DEFAULT = "default";

const char* const XML_ENCODING = "UTF-8";
const char* const XML_VERSION = "1.0";
const char* const XMLATTRIB_COLOR = "color";
const char* const XMLATTRIB_NAME = "name";
const char* const XMLATTRIB_PITCH = "pitch";
const char* const XMLATTRIB_PROGRAM = "program";
const char* const XMLATTRIB_SOUNDFONT = "soundfont";
const char* const XMLATTRIB_TEMPO = "tempo";
const char* const XMLATTRIB_TIME = "time";
const char* const XMLATTRIB_TYPE = "type";
const char* const XMLATTRIB_VELOCITY = "velocity";
const char* const XMLATTRIB_VERSION = "version";
const char* const XMLNODE_BLOCK = "block";
const char* const XMLNODE_BLOCKDEF = "blockdef";
const char* const XMLNODE_BLOCKDEFS = "blockdefs";
const char* const XMLNODE_BLOCKS = "blocks";
const char* const XMLNODE_SCORE = "gscore";
const char* const XMLNODE_MESSAGE = "message";
const char* const XMLNODE_TRACK = "track";
const char* const XMLNODE_TRACKS = "tracks";

enum {
    WINDOW_WIDTH = 640,
    WINDOW_HEIGHT = 640,
    RENDERER_MAX_QUADS = 1000,
    RENDERER_MAX_VERTICES = 4*RENDERER_MAX_QUADS,
    MAX_BLOCKS = 4,
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
    MOUSE_POINTER_HIDDEN=false,
    SYNTH_AUDIO_PERIODS=2,
    SYNTH_AUDIO_PERIOD_SIZE=64,
    SYNTH_MIDI_CHANNELS = N_TRACKS + 1,
    SYNTH_PROGRAM=0,
    SYNTH_ENABLE_REVERB=false,
    SYNTH_ENABLE_CHORUS=false,
    XML_BUFFER_SIZE = 1024,
};

const float CURSOR_SIZE_OFFSET = -0.005f;
const float NOTE_SIZE_OFFSET = -0.002f;
const float BLOCK_SIZE_OFFSET = -0.002f;
const float PLAYER_CURSOR_WIDTH = 0.005f;
const float MAX_TRACK_HEIGHT = 0.1f;
const float SYNTH_GAIN = 1.0f;
const float DEFAULT_VELOCITY = 0.75f;

const Vector4 COLOR_BLOCK_DEFAULT = {0.5411f, 0.7765f, 0.9490f, 1.0f};
const Vector4 COLOR_BACKGROUND = {0.1490f, 0.1961f, 0.2196f, 1.0f};
const Vector4 COLOR_GRIDLINES = {0.1294f, 0.1764, 0.1960, 1.0f};
const Vector4 COLOR_CURSOR = {0.72f, 0.72f, 0.72f, 1.0f};

const char* BLOCK_NAMES[] = {  /* TODO: do not hard-code these (or the number of blocks) */
    "block1",
    "block2",
    "block3",
    "block4",
};

const Vector4 BLOCK_COLORS[] = {
    {0.5411f, 0.7765f, 0.9490f, 1.0f},
    {0.6235f, 0.6588f, 0.8549f, 1.0f},
    {0.7725f, 0.8824f, 0.6471f, 1.0f},
    {1.0000f, 0.8784f, 0.5098f, 1.0f},
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

const char* ATOM_PROMPTS[] =  {
    "Set tempo (BPM):",
    "Set instrument:",
};

char* (*ATOM_FUNCTIONS[ATOM_COUNT])(void) = {
    EditView_getTempoString,
    Synth_getInstrumentListString,
};

const char* const cmdQuery =  /* sprintf(cmdQueryFull, cmdQuery, windowId, prompt, atomName) */
    "export WINDOWID=\"%lu\";"
    "export PROMPT=\"%s\";"
    "export ATOM_NAME=\"%s\";"
    "dmenu -b -i -p \"$PROMPT\" -w \"$WINDOWID\""
    "| tr '\\n' '\\0' "
    "| xargs -r0 xprop -id \"$WINDOWID\" -f \"$ATOM_NAME\" 8s -set \"$ATOM_NAME\"";
