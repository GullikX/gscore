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

const char* const WINDOW_TITLE = "gscore";

const char* const AUDIO_DRIVER = "alsa";
const char* const SOUNDFONT = "soundfont.sf2";

const char* const BLOCK_NAME_DEFAULT = "default";

const char* const XML_ENCODING = "UTF-8";
const char* const XML_VERSION = "1.0";
const char* const XMLATTRIB_COLOR = "color";
const char* const XMLATTRIB_IGNORENOTEOFF = "ignorenoteoff";
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
    WINDOW_WIDTH = 1280,
    WINDOW_HEIGHT = 720,
    RENDERER_MAX_QUADS = 1000,
    RENDERER_MAX_VERTICES = 4*RENDERER_MAX_QUADS,
    MAX_BLOCKS = 64,
    MAX_BLOCK_NAME_LENGTH = 64,
    N_TRACKS_MAX = 64,
    SCORE_LENGTH_DEFAULT = 16,
    SCORE_LENGTH_MAX = 1024,
    BEATS_PER_MEASURE = 4,
    SECONDS_PER_MINUTE = 60,
    TEMPO_BPM = 100,
    TEMPO_BPM_MAX = 1024,
    BLOCK_MEASURES = 4,
    MEASURE_RESOLUTION = 4,
    OCTAVES = 5,
    NOTES_IN_OCTAVE = 12,
    IGNORE_NOTE_OFF_DEFAULT = false,
    EDIT_MODE_PREVIEW_VELOCITY = 50,
    MOUSE_POINTER_HIDDEN = false,
    SYNTH_AUDIO_PERIODS = 2,
    SYNTH_AUDIO_PERIOD_SIZE = 64,
    SYNTH_MIDI_CHANNELS = N_TRACKS_MAX + 1,
    SYNTH_BANK_DEFAULT = 0,
    SYNTH_PROGRAM_DEFAULT = 0,
    SYNTH_PROGRAM_NAME_LENGTH_MAX = 1024,
    MAX_SYNTH_BANKS = 1024,
    MAX_SYNTH_PROGRAMS = 1024,
    SYNTH_ENABLE_REVERB = false,
    SYNTH_ENABLE_CHORUS = false,
    XML_BUFFER_SIZE = 1024,
};

const float CURSOR_SIZE_OFFSET = -0.005f;
const float NOTE_SIZE_OFFSET = -0.002f;
const float BLOCK_SIZE_OFFSET = -0.002f;
const float HIGHLIGHT_STRENGTH = 0.4f;
const float PLAYER_CURSOR_WIDTH = 0.005f;
const float MAX_TRACK_HEIGHT = 0.1f;
const float SYNTH_GAIN = 1.0f;
const float DEFAULT_VELOCITY = 0.75f;
const float EDIT_MODE_PLAYBACK_VELOCITY = DEFAULT_VELOCITY * DEFAULT_VELOCITY;

typedef enum {
    C_MAJOR,

    G_MAJOR,
    D_MAJOR,
    A_MAJOR,
    E_MAJOR,
    B_MAJOR,
    F_SHARP_MAJOR,
    C_SHARP_MAJOR,

    F_MAJOR,
    B_FLAT_MAJOR,
    E_FLAT_MAJOR,
    A_FLAT_MAJOR,
    D_FLAT_MAJOR,
    G_FLAT_MAJOR,
    C_FLAT_MAJOR,

    KEY_SIGNATURE_COUNT,
} KeySignature;

const int KEY_SIGNATURES[KEY_SIGNATURE_COUNT][NOTES_IN_OCTAVE] = {
    // C   C#  D   D#  E   F   F#  G   G#  A   A#  B
    {  1,  0,  1,  0,  1,  1,  0,  1,  0,  1,  0,  1},

    {  1,  0,  1,  0,  1,  0,  1,  1,  0,  1,  0,  1},
    {  0,  1,  1,  0,  1,  0,  1,  1,  0,  1,  0,  1},
    {  0,  1,  1,  0,  1,  0,  1,  0,  1,  1,  0,  1},
    {  0,  1,  0,  1,  1,  0,  1,  0,  1,  1,  0,  1},
    {  0,  1,  0,  1,  1,  0,  1,  0,  1,  0,  1,  1},
    {  0,  1,  0,  1,  0,  1,  1,  0,  1,  0,  1,  1},
    {  1,  1,  0,  1,  0,  1,  1,  0,  1,  0,  1,  0},

    {  1,  0,  1,  0,  1,  1,  0,  1,  0,  1,  1,  0},
    {  1,  0,  1,  1,  0,  1,  0,  1,  0,  1,  1,  0},
    {  1,  0,  1,  1,  0,  1,  0,  1,  1,  0,  1,  0},
    {  1,  1,  0,  1,  0,  1,  0,  1,  1,  0,  1,  0},
    {  1,  1,  0,  1,  0,  1,  1,  0,  1,  0,  1,  0},
    {  0,  1,  0,  1,  0,  1,  1,  0,  1,  0,  1,  1},
    {  0,  1,  0,  1,  1,  0,  1,  0,  1,  0,  1,  1},
};

const char* KEY_SIGNATURE_NAMES[] =  {
    "C major / A minor",

    "G major / E minor",
    "D major / B minor",
    "A major / F-sharp minor",
    "E major / C-sharp minor",
    "B major / G-sharp minor",
    "F-sharp major / D-sharp minor",
    "C-sharp major / A-sharp minor",

    "F major / D minor",
    "B-flat major / G minor",
    "E-flat major / C minor",
    "A-flat major / F minor",
    "D-flat major / B-flat minor",
    "G-flat major / E-flat minor",
    "C-flat major / A-flat minor",
};

const char* const KEY_SIGNATURE_LIST_STRING =
    "C major / A minor\n"

    "G major / E minor\n"
    "D major / B minor\n"
    "A major / F-sharp minor\n"
    "E major / C-sharp minor\n"
    "B major / G-sharp minor\n"
    "F-sharp major / D-sharp minor\n"
    "C-sharp major / A-sharp minor\n"

    "F major / D minor\n"
    "B-flat major / G minor\n"
    "E-flat major / C minor\n"
    "A-flat major / F minor\n"
    "D-flat major / B-flat minor\n"
    "G-flat major / E-flat minor\n"
    "C-flat major / A-flat minor";

KeySignature KEY_SIGNATURE_DEFAULT = C_MAJOR;

const char* const COLOR_BACKGROUND = "263238";
const char* const COLOR_GRIDLINES = "212D32";
const char* const COLOR_CURSOR = "38434A";
const char* const COLOR_PLAYBACK_CURSOR = "C5C8C6";
const char* const COLOR_BLOCK_DEFAULT = "81A2BE";

enum {
    ATOM_BPM,
    ATOM_SYNTH_PROGRAM,
    ATOM_SELECT_BLOCK,
    ATOM_RENAME_BLOCK,
    ATOM_SET_BLOCK_COLOR,
    ATOM_SET_KEY_SIGNATURE,
    ATOM_COUNT,
};

const char* ATOM_NAMES[] =  {
    "_GSCORE_BPM",
    "_GSCORE_SYNTH_PROGRAM",
    "_GSCORE_SELECT_BLOCK",
    "_GSCORE_RENAME_BLOCK",
    "_GSCORE_SET_BLOCK_COLOR",
    "_GSCORE_SET_KEY_SIGNATURE",
};

const char* ATOM_PROMPTS[] =  {
    "Set tempo (BPM):",
    "Set instrument:",
    "Select block:",
    "Rename block:",
    "Set block color:",
    "Set key signature:",
};

const char* (*ATOM_FUNCTIONS[ATOM_COUNT])(void) = {
    EditView_getTempoString,
    Synth_getInstrumentListString,
    Score_getBlockListString,
    Score_getCurrentBlockName,
    Score_getCurrentBlockColor,
    Score_getKeySignatureName,
};

const char* const cmdQuery =  /* sprintf(cmdQueryFull, cmdQuery, windowId, prompt, atomName) */
    "export WINDOWID=\"%lu\";"
    "export PROMPT=\"%s\";"
    "export ATOM_NAME=\"%s\";"
    "dmenu -b -i -p \"$PROMPT\" -w \"$WINDOWID\""
    "| tr '\\n' '\\0' "
    "| xargs -r0 xprop -id \"$WINDOWID\" -f \"$ATOM_NAME\" 8s -set \"$ATOM_NAME\"";
