/* Copyright (C) 2020 Martin Gulliksson <martin@gullik.cc>
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

Synth* Synth_new(void) {
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

    self->soundFontId = fluid_synth_sfload(self->fluidSynth, SOUNDFONT, true);
    self->soundFont = fluid_synth_get_sfont(self->fluidSynth, 0);
    if (self->soundFontId == FLUID_FAILED || !self->soundFont) {
        die("Failed to load soundfont");
    }

    fluid_synth_set_gain(self->fluidSynth, SYNTH_GAIN);
    fluid_synth_set_reverb_on(self->fluidSynth, SYNTH_ENABLE_REVERB);
    fluid_synth_set_chorus_on(self->fluidSynth, SYNTH_ENABLE_CHORUS);

    self->sequencer = new_fluid_sequencer2(0);
    self->synthSequencerId = fluid_sequencer_register_fluidsynth(self->sequencer, self->fluidSynth);
    self->callbackId = fluid_sequencer_register_client(self->sequencer, "me", Synth_sequencerCallback, NULL);

    /* Generate instrument list string */
    int instrumentListStringSize = 1;
    for (int iBank = 0; iBank < MAX_SYNTH_BANKS; iBank++) {
        for (int iProgram = 0; iProgram < MAX_SYNTH_PROGRAMS; iProgram++) {
            fluid_preset_t* preset = fluid_sfont_get_preset(self->soundFont, iBank, iProgram);
            if (!preset) continue;
            instrumentListStringSize += strlen(fluid_preset_get_name(preset)) + 1;
        }
    }
    self->instrumentListString = ecalloc(instrumentListStringSize, sizeof(char));

    for (int iBank = 0; iBank < MAX_SYNTH_BANKS; iBank++) {
        for (int iProgram = 0; iProgram < MAX_SYNTH_PROGRAMS; iProgram++) {
            fluid_preset_t* preset = fluid_sfont_get_preset(self->soundFont, iBank, iProgram);
            if (!preset) continue;
            strcat(self->instrumentListString, fluid_preset_get_name(preset));
            strcat(self->instrumentListString, "\n");
        }
    }

    /* Synth program for editview */
    Synth_setProgramByName(self, 0, Synth_getDefaultProgramName(self));

    return self;
}


Synth* Synth_free(Synth* self) {
    fluid_sequencer_unregister_client(self->sequencer, self->callbackId);
    delete_fluid_sequencer(self->sequencer);
    delete_fluid_audio_driver(self->audioDriver);
    delete_fluid_synth(self->fluidSynth);
    delete_fluid_settings(self->settings);

    free(self->instrumentListString);

    free(self);
    return NULL;
}


void Synth_setProgramById(Synth* self, int channel, int iBank, int iProgram) {
    printf("Synth_setProgramById channel:%d program:(%d, %d)\n", channel, iBank, iProgram);
    if (fluid_synth_program_select(self->fluidSynth, channel, self->soundFontId, iBank, iProgram) == FLUID_FAILED) {
        puts("Error: Failed to set midi program");
    }
}


void Synth_setProgramByName(Synth* self, int channel, const char* const programName) {
    for (int iBank = 0; iBank < MAX_SYNTH_BANKS; iBank++) {
        for (int iProgram = 0; iProgram < MAX_SYNTH_PROGRAMS; iProgram++) {
            fluid_preset_t* preset = fluid_sfont_get_preset(self->soundFont, iBank, iProgram);
            if (!preset) continue;
            if (!strcmp(programName, fluid_preset_get_name(preset))) {
                fluid_synth_program_select(self->fluidSynth, channel, self->soundFontId, iBank, iProgram);
                return;
            }
        }
    }
    printf("Error: Invalid instrument name '%s'\n", programName);
}


void Synth_processMessage(Synth* self, int channel, MidiMessage* midiMessage) {
    int velocity = 127.0f * midiMessage->velocity;
    switch (midiMessage->type) {
        case FLUID_SEQ_NOTE:
            break;
        case FLUID_SEQ_NOTEON:
            fluid_synth_noteon(self->fluidSynth, channel, midiMessage->pitch, velocity);
            break;
        case FLUID_SEQ_NOTEOFF:
            fluid_synth_noteoff(self->fluidSynth, channel, midiMessage->pitch);
            break;
        default:
            printf("Warning: ignoring invalid midi message type %d\n", midiMessage->type);
            break;
    }
}


void Synth_noteOn(Synth* self, int key) {
    fluid_synth_noteon(self->fluidSynth, 0, key, EDIT_MODE_PREVIEW_VELOCITY);
}


void Synth_sendNoteOn(Synth* self, int channel, int pitch, float velocity, float time) {
    int velocityInt = 127.0f * velocity;
    int timeInt = 1000.0f * time;

    fluid_event_t* event = new_fluid_event();
    fluid_event_set_source(event, -1);
    fluid_event_set_dest(event, self->synthSequencerId);
    fluid_event_noteon(event, channel, pitch, velocityInt);

    fluid_sequencer_send_at(self->sequencer, event, timeInt, 0);
    delete_fluid_event(event);
}


void Synth_noteOff(Synth* self, int key) {
    fluid_synth_noteoff(self->fluidSynth, 0, key);
}


void Synth_sendNoteOff(Synth* self, int channel, int pitch, float time) {
    int timeInt = 1000.0f * time;

    fluid_event_t* event = new_fluid_event();
    fluid_event_set_source(event, -1);
    fluid_event_set_dest(event, self->synthSequencerId);
    fluid_event_noteoff(event, channel, pitch);

    fluid_sequencer_send_at(self->sequencer, event, timeInt, 0);
    delete_fluid_event(event);
}


void Synth_noteOffAll(Synth* self) {
    fluid_sequencer_remove_events(self->sequencer, -1, -1, -1);
    for (int iChannel = 0; iChannel < SYNTH_MIDI_CHANNELS; iChannel++) {
        fluid_synth_all_notes_off(self->fluidSynth, iChannel);
    }
}


void Synth_scheduleCallback(Synth* self, float time) {
    int timeInt = 1000.0f * time;

    fluid_event_t* event = new_fluid_event();
    fluid_event_set_source(event, -1);
    fluid_event_set_dest(event, self->callbackId);
    fluid_event_timer(event, NULL);

    fluid_sequencer_send_at(self->sequencer, event, timeInt, 0);
    delete_fluid_event(event);
}


int Synth_getTime(Synth* self) {
    return fluid_sequencer_get_tick(self->sequencer);
}


void Synth_sequencerCallback(unsigned int time, fluid_event_t* event, fluid_sequencer_t* sequencer, void* data) {
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


const char* Synth_getDefaultProgramName(Synth* self) {
    fluid_preset_t* preset = fluid_sfont_get_preset(self->soundFont, SYNTH_BANK_DEFAULT, SYNTH_PROGRAM_DEFAULT);
    if (!preset) die("Could not determine default program name");
    const char* name = fluid_preset_get_name(preset);
    if (!name) die("Could not determine default program name");

    return name;
}


const char* Synth_getInstrumentListString(void) {  /* called from input callback (no instance reference) */
    return Application_getInstance()->synth->instrumentListString;
}
