Synth* Synth_getInstance(void) {
    static Synth* self = NULL;
    if (self) return self;

    self = ecalloc(1, sizeof(*self));

    self->settings = new_fluid_settings();
    self->fluidSynth = new_fluid_synth(self->settings);
    fluid_settings_setstr(self->settings, "audio.driver", AUDIO_DRIVER);
    fluid_settings_setstr(self->settings, "audio.alsa.device", ALSA_DEVICE);
    fluid_settings_setint(self->settings, "synth.midi-channels", SYNTH_MIDI_CHANNELS);
    fluid_settings_setint(self->settings, "audio.periods", SYNTH_AUDIO_PERIODS);
    fluid_settings_setint(self->settings, "audio.period-size", SYNTH_AUDIO_PERIOD_SIZE);

    self->audioDriver = new_fluid_audio_driver(self->settings, self->fluidSynth);
    if (!self->audioDriver) {
        die("Failed to initialize FluidSynth");
    }

    if (fluid_synth_sfload(self->fluidSynth, SOUNDFONT, true) == FLUID_FAILED) {
        die("Failed to load soundfont");
    }

    Synth_setProgramById(0, SYNTH_PROGRAM);
    fluid_synth_set_gain(self->fluidSynth, SYNTH_GAIN);
    fluid_synth_set_reverb_on(self->fluidSynth, SYNTH_ENABLE_REVERB);
    fluid_synth_set_chorus_on(self->fluidSynth, SYNTH_ENABLE_CHORUS);

    /* Generate instrument list string */
    fluid_sfont_t *soundFont = fluid_synth_get_sfont(self->fluidSynth, 0);
    if (!soundFont) {
        die("Soundfont pointer is null");
    }
    int instrumentListStringSize = 1;
    int nInstruments = 0;
    for (int i = 0;; i++) {
        fluid_preset_t* preset = fluid_sfont_get_preset(soundFont, 0, i);
        if (!preset) break;
        nInstruments = i + 1;
        instrumentListStringSize += strlen(fluid_preset_get_name(preset)) + 1;
    }
    self->instrumentListString = ecalloc(instrumentListStringSize, sizeof(char));
    for (int i = 0; i < nInstruments; i++) {
        if (i > 0) {
            strcat(self->instrumentListString, "\n");
        }
        fluid_preset_t* preset = fluid_sfont_get_preset(soundFont, 0, i);
        if (!preset) break;
        strcat(self->instrumentListString, fluid_preset_get_name(preset));
    }

    return self;
}


void Synth_setProgramById(int channel, int program) {
    printf("setProgramById %d\n", program);
    Synth* self = Synth_getInstance();
    if (fluid_synth_program_change(self->fluidSynth, channel, program) == FLUID_FAILED) {
        puts("Error: Failed to set midi program");
    }
}


void Synth_setProgramByName(int channel, const char* const instrumentName) {
    Synth* self = Synth_getInstance();

    fluid_sfont_t *soundFont = fluid_synth_get_sfont(self->fluidSynth, 0);
    if (!soundFont) {
        die("Soundfont pointer is null");
    }

    for (int i = 0;; i++) {
        fluid_preset_t* preset = fluid_sfont_get_preset(soundFont, 0, i);
        if (!preset) break;
        if (!strcmp(instrumentName, fluid_preset_get_name(preset))) {
            printf("Setting instrument to '%s'\n", instrumentName);
            Synth_setProgramById(channel, i);
            return;
        }
    }

    printf("Error: Invalid instrument name '%s'\n", instrumentName);
}


void Synth_noteOn(int key) {
    Synth* self = Synth_getInstance();
    fluid_synth_noteon(self->fluidSynth, 0, key, 100);
}


void Synth_noteOff(int key) {
    Synth* self = Synth_getInstance();
    fluid_synth_noteoff(self->fluidSynth, 0, key);
}


void Synth_noteOffAll(void) {
    Synth* self = Synth_getInstance();
    fluid_synth_all_notes_off(self->fluidSynth, 0);
}


char* Synth_getInstrumentListString(void) {
    Synth* self = Synth_getInstance();
    return self->instrumentListString;
}
