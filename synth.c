/* Copyright (C) 2020-2021 Martin Gulliksson <martin@gullik.cc>
 *
 * This file is part of gscore.
 *
 * gscore is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, version 3.
 *
 * gscore is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>
 *
 */

static Synth* Synth_new(void) {
    Synth* self = ecalloc(1, sizeof(*self));

    self->settings = new_fluid_settings();
    self->fluidSynth = new_fluid_synth(self->settings);
    fluid_settings_setstr(self->settings, "audio.driver", AUDIO_DRIVER);
    fluid_settings_setint(self->settings, "synth.midi-channels", SYNTH_MIDI_CHANNELS);
    fluid_settings_setint(self->settings, "audio.periods", SYNTH_AUDIO_PERIODS);
    fluid_settings_setint(self->settings, "audio.period-size", SYNTH_AUDIO_PERIOD_SIZE);

    self->audioDriver = new_fluid_audio_driver(self->settings, self->fluidSynth);
    if (!self->audioDriver) {
        die("Failed to initialize FluidSynth");
    }

    self->nSoundFonts = 0;
    for (const char** soundFont = SOUNDFONTS; *soundFont; soundFont++) {
        if (self->nSoundFonts >= MAX_SOUNDFONTS) {
            die("Maximum number of soundfonts reached (%d)", MAX_SOUNDFONTS);
        }
        printf("Loading soundfont '%s'...\n", *soundFont);
        self->soundFontIds[self->nSoundFonts] = fluid_synth_sfload(self->fluidSynth, *soundFont, true);
        if (self->soundFontIds[self->nSoundFonts] == FLUID_FAILED) {
            die("Failed to load soundfont '%s'", *soundFont);
        }
        printf("Successfully loaded '%s'\n", *soundFont);
        self->nSoundFonts++;
    }

    fluid_synth_set_gain(self->fluidSynth, SYNTH_GAIN);
    fluid_synth_set_reverb_on(self->fluidSynth, SYNTH_ENABLE_REVERB);
    fluid_synth_set_chorus_on(self->fluidSynth, SYNTH_ENABLE_CHORUS);

    self->sequencer = new_fluid_sequencer2(0);
    self->synthSequencerId = fluid_sequencer_register_fluidsynth(self->sequencer, self->fluidSynth);
    self->callbackId = fluid_sequencer_register_client(self->sequencer, "me", Synth_sequencerCallback, NULL);

    /* Generate instrument list */
    int nInstruments = 0;
    int instrumentListStringSize = 1;

    for (int iSoundFont = 0; iSoundFont < self->nSoundFonts; iSoundFont++) {
        for (int iBank = 0; iBank < MAX_SYNTH_BANKS; iBank++) {
            for (int iProgram = 0; iProgram < MAX_SYNTH_PROGRAMS; iProgram++) {
                fluid_sfont_t* soundFont = fluid_synth_get_sfont_by_id(self->fluidSynth, self->soundFontIds[iSoundFont]);
                fluid_preset_t* preset = fluid_sfont_get_preset(soundFont, iBank, iProgram);
                if (!preset) continue;
                nInstruments++;
                instrumentListStringSize += strlen(fluid_preset_get_name(preset)) + 1;
            }
        }
    }
    self->instrumentMap = HashMap_new(nInstruments);
    self->instrumentListString = ecalloc(instrumentListStringSize, sizeof(char));

    for (int iSoundFont = 0; iSoundFont < self->nSoundFonts; iSoundFont++) {
        for (int iBank = 0; iBank < MAX_SYNTH_BANKS; iBank++) {
            for (int iProgram = 0; iProgram < MAX_SYNTH_PROGRAMS; iProgram++) {
                fluid_sfont_t* soundFont = fluid_synth_get_sfont_by_id(self->fluidSynth, self->soundFontIds[iSoundFont]);
                fluid_preset_t* preset = fluid_sfont_get_preset(soundFont, iBank, iProgram);
                if (!preset) continue;
                const char* name = fluid_preset_get_name(preset);
                int encodedProgram = Synth_encodeProgram(iSoundFont, iBank, iProgram);
                HashMap_put(self->instrumentMap, name, encodedProgram);
                strcat(self->instrumentListString, name);
                strcat(self->instrumentListString, "\n");
            }
        }
    }

    /* Synth program for editview */
    Synth_setProgramByName(self, 0, Synth_getDefaultProgramName(self));

    return self;
}


static Synth* Synth_free(Synth* self) {
    fluid_sequencer_unregister_client(self->sequencer, self->callbackId);
    delete_fluid_sequencer(self->sequencer);
    delete_fluid_audio_driver(self->audioDriver);
    delete_fluid_synth(self->fluidSynth);
    delete_fluid_settings(self->settings);

    HashMap_free(self->instrumentMap);
    free(self->instrumentListString);

    free(self);
    return NULL;
}


static int Synth_encodeProgram(int iSoundFont, int iBank, int iProgram) {
    /* TODO: check for signed integer overflow */
    return iProgram + iBank*MAX_SYNTH_PROGRAMS + iSoundFont*MAX_SYNTH_PROGRAMS*MAX_SYNTH_BANKS;
}


static void Synth_decodeProgram(int encodedProgram, int* iSoundFontOut, int* iBankOut, int* iProgramOut) {
    *iProgramOut = modulo(encodedProgram, MAX_SYNTH_PROGRAMS);
    *iBankOut = modulo(encodedProgram / MAX_SYNTH_PROGRAMS, MAX_SYNTH_BANKS);
    *iSoundFontOut = encodedProgram / (MAX_SYNTH_BANKS * MAX_SYNTH_PROGRAMS);
}


static void Synth_setProgramByName(Synth* self, int channel, const char* const programName) {
    int encodedProgram = HashMap_get(self->instrumentMap, programName);
    if (encodedProgram < 0) {
        printf("Error: Invalid instrument name '%s'\n", programName);
        return;
    }
    int iSoundFont = 0;
    int iBank = 0;
    int iProgram = 0;
    Synth_decodeProgram(encodedProgram, &iSoundFont, &iBank, &iProgram);
    fluid_synth_program_select(self->fluidSynth, channel, self->soundFontIds[iSoundFont], iBank, iProgram);
}


static void Synth_noteOn(Synth* self, int key) {
    fluid_synth_noteon(self->fluidSynth, 0, key, EDIT_MODE_PREVIEW_VELOCITY);
}


static void Synth_sendNoteOn(Synth* self, int channel, int pitch, float velocity, float time) {
    int velocityInt = 127.0f * velocity;
    int timeInt = 1000.0f * time;

    fluid_event_t* event = new_fluid_event();
    fluid_event_set_source(event, -1);
    fluid_event_set_dest(event, self->synthSequencerId);
    fluid_event_noteon(event, channel, pitch, velocityInt);

    fluid_sequencer_send_at(self->sequencer, event, timeInt, 0);
    delete_fluid_event(event);
}


static void Synth_sendNoteOff(Synth* self, int channel, int pitch, float time) {
    int timeInt = 1000.0f * time;

    fluid_event_t* event = new_fluid_event();
    fluid_event_set_source(event, -1);
    fluid_event_set_dest(event, self->synthSequencerId);
    fluid_event_noteoff(event, channel, pitch);

    fluid_sequencer_send_at(self->sequencer, event, timeInt, 0);
    delete_fluid_event(event);
}


static void Synth_noteOffAll(Synth* self) {
    fluid_sequencer_remove_events(self->sequencer, -1, -1, -1);
    for (int iChannel = 0; iChannel < SYNTH_MIDI_CHANNELS; iChannel++) {
        fluid_synth_all_notes_off(self->fluidSynth, iChannel);
    }
}


static void Synth_scheduleCallback(Synth* self, float time) {
    int timeInt = 1000.0f * time;

    fluid_event_t* event = new_fluid_event();
    fluid_event_set_source(event, -1);
    fluid_event_set_dest(event, self->callbackId);
    fluid_event_timer(event, NULL);

    fluid_sequencer_send_at(self->sequencer, event, timeInt, 0);
    delete_fluid_event(event);
}


static int Synth_getTime(Synth* self) {
    return fluid_sequencer_get_tick(self->sequencer);
}


static void Synth_sequencerCallback(unsigned int time, fluid_event_t* event, fluid_sequencer_t* sequencer, void* data) {
    (void)time; (void)sequencer; (void)data;
    if (fluid_event_get_type(event) != FLUID_SEQ_TIMER) return;

    Application* application = Application_getInstance();
    switch (application->state) {
        case OBJECT_MODE:
            ObjectView_sequencerCallback(application->objectView);
            break;
        case EDIT_MODE:
            EditView_sequencerCallback(application->editView);
            break;
    }
}


static const char* Synth_getDefaultProgramName(Synth* self) {
    fluid_sfont_t* soundFont = fluid_synth_get_sfont_by_id(self->fluidSynth, self->soundFontIds[0]);
    fluid_preset_t* preset = fluid_sfont_get_preset(soundFont, SYNTH_BANK_DEFAULT, SYNTH_PROGRAM_DEFAULT);
    if (!preset) die("Could not determine default program name");
    const char* name = fluid_preset_get_name(preset);
    if (!name) die("Could not determine default program name");

    return name;
}


static const char* Synth_getInstrumentListString(void) {
    return Application_getInstance()->synth->instrumentListString;
}
