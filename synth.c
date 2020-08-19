Synth* Synth_getInstance() {
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

    Synth_setProgram(0, SYNTH_PROGRAM);

    fluid_synth_set_reverb_on(self->fluidSynth, SYNTH_ENABLE_REVERB);
    fluid_synth_set_chorus_on(self->fluidSynth, SYNTH_ENABLE_CHORUS);

    return self;
}


void Synth_setProgram(int channel, int program) {
    Synth* self = Synth_getInstance();
    if (fluid_synth_program_change(self->fluidSynth, channel, program) == FLUID_FAILED) {
        puts("Error: Failed to set midi program");
    }
}


void Synth_noteOn(int key) {
    Synth* self = Synth_getInstance();
    fluid_synth_noteon(self->fluidSynth, 0, key, 100);
}


void Synth_noteOffAll() {
    Synth* self = Synth_getInstance();
    for (int i = 0; i < 128; i++) {
        fluid_synth_noteoff(self->fluidSynth, 0, i);
    }
}
