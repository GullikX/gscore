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

    if (fluid_synth_sfload(self->fluidSynth, SOUNDFONT, true) == FLUID_FAILED) {
        die("Failed to load soundfont");
    }

    fluid_synth_set_gain(self->fluidSynth, SYNTH_GAIN);
    fluid_synth_set_reverb_on(self->fluidSynth, SYNTH_ENABLE_REVERB);
    fluid_synth_set_chorus_on(self->fluidSynth, SYNTH_ENABLE_CHORUS);

    self->sequencer = new_fluid_sequencer2(0);
    self->synthSequencerId = fluid_sequencer_register_fluidsynth(self->sequencer, self->fluidSynth);

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

    /* Synth program for editview */
    Synth_setProgramById(self, 0, SYNTH_PROGRAM_DEFAULT);

    return self;
}


Synth* Synth_free(Synth* self) {
    delete_fluid_sequencer(self->sequencer);
    delete_fluid_audio_driver(self->audioDriver);
    delete_fluid_synth(self->fluidSynth);

    free(self);
    return NULL;
}


void Synth_setProgramById(Synth* self, int channel, int program) {
    printf("Synth_setProgramById channel:%d program:%d\n", channel, program);
    if (fluid_synth_program_change(self->fluidSynth, channel, program) == FLUID_FAILED) {
        puts("Error: Failed to set midi program");
    }
}


int Synth_instrumentNameToId(Synth* self, const char* const instrumentName) {
    fluid_sfont_t *soundFont = fluid_synth_get_sfont(self->fluidSynth, 0);
    if (!soundFont) {
        die("Soundfont pointer is null");
    }

    for (int i = 0;; i++) {
        fluid_preset_t* preset = fluid_sfont_get_preset(soundFont, 0, i);
        if (!preset) break;
        if (!strcmp(instrumentName, fluid_preset_get_name(preset))) {
            return i;
        }
    }

    printf("Error: Invalid instrument name '%s'\n", instrumentName);
    return -1;

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
    fluid_synth_noteon(self->fluidSynth, 0, key, 100);
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


int Synth_getTime(Synth* self) {
    return fluid_sequencer_get_tick(self->sequencer);
}


char* Synth_getInstrumentListString(void) {
    return Application_getInstance()->synth->instrumentListString;
}
