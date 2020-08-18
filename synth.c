Synth* Synth_getInstance() {
    static Synth* self = NULL;
    if (self) return self;

    self = ecalloc(1, sizeof(*self));

    self->settings = new_fluid_settings();
    self->fluidSynth = new_fluid_synth(self->settings);
    fluid_settings_setstr(self->settings, "audio.driver", AUDIO_DRIVER);
    fluid_settings_setstr(self->settings, "audio.alsa.device", ALSA_DEVICE);
    self->audioDriver = new_fluid_audio_driver(self->settings, self->fluidSynth);
    if (!self->audioDriver) {
        die("Failed to initialize FluidSynth");
    }

    if (fluid_synth_sfload(self->fluidSynth, SOUNDFRONT, true) == FLUID_FAILED) {
        die("Failed to load soundfront");
    }

    return self;
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
