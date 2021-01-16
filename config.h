static const char* const VERTEX_SHADER_SOURCE =
    "#version 120\n"
    "attribute vec4 color;"
    "void main() {"
    "   gl_FrontColor = color;"
    "   gl_Position = gl_Vertex;"
    "}";

static const char* const FRAGMENT_SHADER_SOURCE =
    "#version 120\n"
    "void main() {"
    "    gl_FragColor = gl_Color;"
    "}";

static const char* const WINDOW_TITLE = "gscore";

static const char* const AUDIO_DRIVER = "alsa";
static const char* SOUNDFONTS[] = {
    "soundfont0.sf2",
    "soundfont1.sf2",
    NULL,
};

static const char* const BLOCK_NAME_DEFAULT = "default";

static const char* const XML_ENCODING = "UTF-8";
static const char* const XML_VERSION = "1.0";
static const char* const XMLATTRIB_BEATSPERMEASURE = "beatspermeasure";
static const char* const XMLATTRIB_COLOR = "color";
static const char* const XMLATTRIB_IGNORENOTEOFF = "ignorenoteoff";
static const char* const XMLATTRIB_KEYSIGNATURE = "keysignature";
static const char* const XMLATTRIB_NAME = "name";
static const char* const XMLATTRIB_PITCH = "pitch";
static const char* const XMLATTRIB_PROGRAM = "program";
static const char* const XMLATTRIB_SOUNDFONT = "soundfont";
static const char* const XMLATTRIB_TEMPO = "tempo";
static const char* const XMLATTRIB_TIME = "time";
static const char* const XMLATTRIB_TYPE = "type";
static const char* const XMLATTRIB_VELOCITY = "velocity";
static const char* const XMLATTRIB_VERSION = "version";
static const char* const XMLNODE_BLOCK = "block";
static const char* const XMLNODE_BLOCKDEF = "blockdef";
static const char* const XMLNODE_BLOCKDEFS = "blockdefs";
static const char* const XMLNODE_BLOCKS = "blocks";
static const char* const XMLNODE_GSCORE = "gscore";
static const char* const XMLNODE_MESSAGE = "message";
static const char* const XMLNODE_METADATA = "metadata";
static const char* const XMLNODE_SCORE = "score";
static const char* const XMLNODE_TRACK = "track";
static const char* const XMLNODE_TRACKS = "tracks";

enum {
    WINDOW_WIDTH = 1280,
    WINDOW_HEIGHT = 720,
    RENDERER_MAX_QUADS = 1000,
    RENDERER_MAX_VERTICES = 4*RENDERER_MAX_QUADS,
    MAX_BLOCKS = 1024,
    MAX_BLOCK_NAME_LENGTH = 1024,
    N_TRACKS_MAX = 255,
    SCORE_LENGTH_DEFAULT = 16,
    SCORE_LENGTH_MAX = 1024,
    BEATS_PER_MEASURE_DEFAULT = 4,
    SECONDS_PER_MINUTE = 60,
    TEMPO_BPM = 120,
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
    MAX_SOUNDFONTS = 64,
    MAX_SYNTH_BANKS = 1024,
    MAX_SYNTH_PROGRAMS = 1024,
    SYNTH_ENABLE_REVERB = false,
    SYNTH_ENABLE_CHORUS = false,
    XML_BUFFER_SIZE = 1024,
};

static const float CURSOR_SIZE_OFFSET = -0.005f;
static const float NOTE_SIZE_OFFSET = -0.002f;
static const float BLOCK_SIZE_OFFSET = -0.002f;
static const float HIGHLIGHT_STRENGTH = 0.4f;
static const float PLAYER_CURSOR_WIDTH = 0.005f;
static const float MAX_TRACK_HEIGHT = 0.1f;
static const float SYNTH_GAIN = 1.0f;
static const float DEFAULT_VELOCITY = 0.75f;
static const float VELOCITY_MIN = 0.0f;
static const float VELOCITY_MAX = 1.0f;
static const float VELOCITY_ADJUSTMENT_AMOUNT = 0.01f;
static const float EDIT_MODE_PLAYBACK_VELOCITY = DEFAULT_VELOCITY * DEFAULT_VELOCITY;
static const float VELOCITY_INDICATOR_WIDTH_OBJECT_MODE = 0.07f;
static const float VELOCITY_INDICATOR_WIDTH_EDIT_MODE = 0.4f;

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

static const int KEY_SIGNATURES[KEY_SIGNATURE_COUNT][NOTES_IN_OCTAVE] = {
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

static const char* KEY_SIGNATURE_NAMES[] =  {
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

static const char* const KEY_SIGNATURE_LIST_STRING =
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

static const char* NOTE_NAMES[] = {
    "C",
    "C#",
    "D",
    "D#",
    "E",
    "F",
    "F#",
    "G",
    "G#",
    "A",
    "A#",
    "B",
};

static const char* const COLOR_BACKGROUND = "263238";
static const char* const COLOR_GRIDLINES = "212D32";
static const char* const COLOR_CURSOR = "38434A";
static const char* const COLOR_PLAYBACK_CURSOR = "C5C8C6";
static const char* const COLOR_BLOCK_DEFAULT = "81A2BE";

enum {
    ATOM_BPM,
    ATOM_SYNTH_PROGRAM,
    ATOM_SELECT_BLOCK,
    ATOM_RENAME_BLOCK,
    ATOM_SET_BLOCK_COLOR,
    ATOM_SET_KEY_SIGNATURE,
    ATOM_SET_TRACK_VELOCITY,
    ATOM_QUIT,
    ATOM_COUNT,
};

static const char* ATOM_NAMES[] =  {
    "_GSCORE_BPM",
    "_GSCORE_SYNTH_PROGRAM",
    "_GSCORE_SELECT_BLOCK",
    "_GSCORE_RENAME_BLOCK",
    "_GSCORE_SET_BLOCK_COLOR",
    "_GSCORE_SET_KEY_SIGNATURE",
    "_GSCORE_SET_TRACK_VELOCITY",
    "_GSCORE_QUIT",
};

static const char* ATOM_PROMPTS[] =  {
    "Set tempo (BPM):",
    "Set instrument:",
    "Select block:",
    "Rename block:",
    "Set block color:",
    "Set key signature:",
    "Set track velocity:",
    "Quit?",
};

static const char* (*ATOM_FUNCTIONS[ATOM_COUNT])(void) = {
    Score_getTempoString,
    Synth_getInstrumentListString,
    Score_getBlockListString,
    Score_getCurrentBlockName,
    Score_getCurrentBlockColor,
    Score_getKeySignatureName,
    ObjectView_getTrackVelocityString,
    returnYes,
};

static const char* YES = "Yes";

static const char* const cmdQuery =  /* sprintf(cmdQueryFull, cmdQuery, windowId, prompt, atomName) */
    "export WINDOWID=\"%lu\";"
    "export PROMPT=\"%s\";"
    "export ATOM_NAME=\"%s\";"
    "dmenu -b -i -p \"$PROMPT\" -w \"$WINDOWID\""
    "| tr '\\n' '\\0' "
    "| xargs -r0 xprop -id \"$WINDOWID\" -f \"$ATOM_NAME\" 8s -set \"$ATOM_NAME\"";
