/* Copyright (C) 2020-2024 Martin Gulliksson <martin@gullik.cc>
 *
 * This file is part of gscore.
 *
 * gscore is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 3.
 *
 * gscore is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>
 *
 */

#include "synth.h"

#include "common/constants/fluidmidi.h"
#include "common/structs/midimessage.h"
#include "common/structs/queryrequest.h"
#include "common/structs/queryresult.h"
#include "common/structs/sequencerrequest.h"
#include "common/structs/synthprogramchange.h"
#include "common/util/alloc.h"
#include "common/util/log.h"
#include "common/util/stringmap.h"
#include "config/config.h"
#include "events/events.h"

#include <fluidsynth.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static const char* const AUDIO_DRIVER = "alsa";
static const char* const ENVVAR_SOUNDFONTS = "GSCORE_SOUNDFONTS";
static const char* const SOUNDFONTS_DELIMITER = ":";

static const float SYNTH_GAIN = 1.0f;

static const char* const EVENT_SEQUENCER_CALLBACK = "EVENT_SEQUENCER_CALLBACK";
static const char* const REQUEST_KEY_CHANGE_SYNTH_INSTRUMENT = "REQUEST_KEY_CHANGE_SYNTH_INSTRUMENT";


enum {
    MAX_SOUNDFONTS = 64,
    SYNTH_ENABLE_REVERB = false,
    SYNTH_ENABLE_CHORUS = false,
    SYNTH_AUDIO_PERIODS = 2,
    SYNTH_AUDIO_PERIOD_SIZE = 64,
    SYNTH_MIDI_CHANNELS = 16,
    FX_GROUP_ALL = -1,
};


typedef struct SynthInstrument SynthInstrument;
struct SynthInstrument {
    size_t iSoundFont;
    int iProgram;
    int iBank;
};


struct Synth {
    Score* score;
    fluid_settings_t* settings;
    fluid_synth_t* fluidSynth;
    fluid_audio_driver_t* audioDriver;
    int soundFontIds[MAX_SOUNDFONTS];
    size_t nSoundFonts;
    fluid_sequencer_t* sequencer;
    fluid_seq_id_t synthSequencerId;
    fluid_seq_id_t callbackId;
    bool isSequencerRunning;
    unsigned int sequencerStartTimeTicks;
    unsigned int sequencerEndTimeTicks;
    float sequencerInitialProgressFraction;
    SynthInstrument* synthInstruments;
    char* synthInstrumentListString;
    StringMap* synthInstrumentMap;
    int lastRequestedSynthInstrumentChangeChannel;
};


static void Synth_onProcessFrame(Synth* self, void* sender, float* deltaTime);
static void Synth_onQueryResult(Synth* self, void* sender, QueryResult* queryResult);
static void Synth_onRequestChangeSynthInstrument(Synth* self, void* sender, SynthProgramChange* synthProgramChange);
static void Synth_onRequestChangeSynthInstrumentQuery(Synth* self, void* sender, int* iChannel);
static void Synth_onRequestMidiMessagePlay(Synth* self, void* sender, MidiMessage* midiMessage);
static void Synth_onRequestMidiChannelStop(Synth* self, void* sender, int* iChannel);
static void Synth_onRequestSequencerStart(Synth* self, void* sender, SequencerRequest* sequencerRequest);
static void Synth_onSequencerStarted(Synth* self, void* sender, SequencerRequest* sequencerRequest);
static void Synth_onRequestSequencerStop(Synth* self, void* sender, void* unused);
static void Synth_onSequencerCallback(Synth* self, void* sender, void* unused);
static void Synth_setSynthProgram(Synth* self, const char* synthProgramName, int iChannel);
static void Synth_sequencerCallback(unsigned int time, fluid_event_t* event, fluid_sequencer_t* sequencer, void* data);


Synth* Synth_new(Score* score) {
    Synth* self = ecalloc(1, sizeof(*self));

    self->score = score;

    const char* soundFonts = getenv(ENVVAR_SOUNDFONTS);
    if (!soundFonts) {
        Log_fatal("Environment variable %s is not set", ENVVAR_SOUNDFONTS);
    }

    self->settings = new_fluid_settings();
    self->fluidSynth = new_fluid_synth(self->settings);
    fluid_settings_setstr(self->settings, "audio.driver", AUDIO_DRIVER);
    fluid_settings_setint(self->settings, "synth.midi-channels", SYNTH_MIDI_CHANNELS);
    fluid_settings_setint(self->settings, "audio.periods", SYNTH_AUDIO_PERIODS);
    fluid_settings_setint(self->settings, "audio.period-size", SYNTH_AUDIO_PERIOD_SIZE);

    self->audioDriver = new_fluid_audio_driver(self->settings, self->fluidSynth);
    if (!self->audioDriver) {
        Log_fatal("Failed to initialize FluidSynth");
    }

    {
        self->nSoundFonts = 0;
        char* soundFontsDuped = estrdup(soundFonts);
        for (char* soundFont = strtok(soundFontsDuped, SOUNDFONTS_DELIMITER); soundFont; soundFont = strtok(NULL, SOUNDFONTS_DELIMITER)) {
            if (self->nSoundFonts >= MAX_SOUNDFONTS) {
                Log_fatal("Maximum number of soundfonts reached (%d)", MAX_SOUNDFONTS);
            }
            Log_info("Loading soundfont '%s'...", soundFont);
            self->soundFontIds[self->nSoundFonts] = fluid_synth_sfload(self->fluidSynth, soundFont, true);
            if (self->soundFontIds[self->nSoundFonts] == FLUID_FAILED) {
                Log_fatal("Failed to load soundfont '%s'", soundFont);
            }
            else {
                Log_info("Successfully loaded '%s'", soundFont);
                self->nSoundFonts++;
            }
        }
        sfree((void**)&soundFontsDuped);
    }

    fluid_synth_set_gain(self->fluidSynth, SYNTH_GAIN);
    fluid_synth_reverb_on(self->fluidSynth, FX_GROUP_ALL, SYNTH_ENABLE_REVERB);
    fluid_synth_chorus_on(self->fluidSynth, FX_GROUP_ALL, SYNTH_ENABLE_CHORUS);

    self->sequencer = new_fluid_sequencer2(0);
    self->synthSequencerId = fluid_sequencer_register_fluidsynth(self->sequencer, self->fluidSynth);
    self->callbackId = fluid_sequencer_register_client(self->sequencer, "me", Synth_sequencerCallback, NULL);

    /* Parse synth instruments */
    size_t nSynthInstruments = 0;
    size_t synthInstrumentListStringLength = 1;  // space for null-terminator

    for (size_t iSoundFont = 0; iSoundFont < self->nSoundFonts; iSoundFont++) {
        fluid_sfont_t* soundFont = fluid_synth_get_sfont_by_id(self->fluidSynth, self->soundFontIds[iSoundFont]);
        fluid_preset_t* preset;
        fluid_sfont_iteration_start(soundFont);
        while ((preset = fluid_sfont_iteration_next(soundFont))) {
            nSynthInstruments++;

            const char* synthInstrumentName = fluid_preset_get_name(preset);
            size_t synthInstrumentNameLength = strlen(synthInstrumentName);
            Log_assert(synthInstrumentNameLength <= MIDI_SYNTH_PROGRAM_NAME_LENGTH_MAX, "Synth instrument name too long: '%s'", synthInstrumentName);

            synthInstrumentListStringLength += synthInstrumentNameLength;
            synthInstrumentListStringLength += strlen("\n");
        }
    }

    self->synthInstruments = ecalloc(nSynthInstruments, sizeof(SynthInstrument));
    self->synthInstrumentListString = ecalloc(synthInstrumentListStringLength, sizeof(char));
    self->synthInstrumentMap = StringMap_new(MIDI_SYNTH_PROGRAM_NAME_LENGTH_MAX);

    size_t iSynthInstrument = 0;
    for (size_t iSoundFont = 0; iSoundFont < self->nSoundFonts; iSoundFont++) {
        fluid_sfont_t* soundFont = fluid_synth_get_sfont_by_id(self->fluidSynth, self->soundFontIds[iSoundFont]);
        fluid_preset_t* preset;
        fluid_sfont_iteration_start(soundFont);
        while ((preset = fluid_sfont_iteration_next(soundFont))) {
            const char* synthInstrumentName = fluid_preset_get_name(preset);
            strcat(self->synthInstrumentListString, synthInstrumentName);
            strcat(self->synthInstrumentListString, "\n");

            SynthInstrument synthInstrument = {
                .iSoundFont = self->soundFontIds[iSoundFont],
                .iBank = fluid_preset_get_banknum(preset),
                .iProgram = fluid_preset_get_num(preset),
            };
            self->synthInstruments[iSynthInstrument] = synthInstrument;

            if (!StringMap_containsItem(self->synthInstrumentMap, synthInstrumentName)) {
                StringMap_addItem(self->synthInstrumentMap, synthInstrumentName, &self->synthInstruments[iSynthInstrument]);
            }

            iSynthInstrument++;
        }
    }

    Event_subscribe(EVENT_PROCESS_FRAME, self, EVENT_CALLBACK(Synth_onProcessFrame), sizeof(float));
    Event_subscribe(EVENT_QUERY_RESULT, self, EVENT_CALLBACK(Synth_onQueryResult), sizeof(QueryResult));
    Event_subscribe(EVENT_REQUEST_CHANGE_SYNTH_INSTRUMENT, self, EVENT_CALLBACK(Synth_onRequestChangeSynthInstrument), sizeof(SynthProgramChange));
    Event_subscribe(EVENT_REQUEST_CHANGE_SYNTH_INSTRUMENT_QUERY, self, EVENT_CALLBACK(Synth_onRequestChangeSynthInstrumentQuery), sizeof(int));
    Event_subscribe(EVENT_REQUEST_MIDI_MESSAGE_PLAY, self, EVENT_CALLBACK(Synth_onRequestMidiMessagePlay), sizeof(MidiMessage));
    Event_subscribe(EVENT_REQUEST_MIDI_CHANNEL_STOP, self, EVENT_CALLBACK(Synth_onRequestMidiChannelStop), sizeof(int));
    Event_subscribe(EVENT_REQUEST_SEQUENCER_START, self, EVENT_CALLBACK(Synth_onRequestSequencerStart), sizeof(SequencerRequest));
    Event_subscribe(EVENT_REQUEST_SEQUENCER_STOP, self, EVENT_CALLBACK(Synth_onRequestSequencerStop), 0);
    Event_subscribe(EVENT_SEQUENCER_STARTED, self, EVENT_CALLBACK(Synth_onSequencerStarted), sizeof(SequencerRequest));

    Events_defineNewLocalEventType(EVENT_SEQUENCER_CALLBACK, 0, self);
    Event_subscribe(EVENT_SEQUENCER_CALLBACK, self, EVENT_CALLBACK(Synth_onSequencerCallback), 0);

    Score_requestSynthPrograms(self->score);

    return self;
}


void Synth_free(Synth** pself) {
    Synth* self = *pself;

    Event_unsubscribe(EVENT_PROCESS_FRAME, self, EVENT_CALLBACK(Synth_onProcessFrame), sizeof(float));
    Event_unsubscribe(EVENT_QUERY_RESULT, self, EVENT_CALLBACK(Synth_onQueryResult), sizeof(QueryResult));
    Event_unsubscribe(EVENT_REQUEST_CHANGE_SYNTH_INSTRUMENT, self, EVENT_CALLBACK(Synth_onRequestChangeSynthInstrument), sizeof(SynthProgramChange));
    Event_unsubscribe(EVENT_REQUEST_CHANGE_SYNTH_INSTRUMENT_QUERY, self, EVENT_CALLBACK(Synth_onRequestChangeSynthInstrumentQuery), sizeof(int));
    Event_unsubscribe(EVENT_REQUEST_MIDI_MESSAGE_PLAY, self, EVENT_CALLBACK(Synth_onRequestMidiMessagePlay), sizeof(MidiMessage));
    Event_unsubscribe(EVENT_REQUEST_MIDI_CHANNEL_STOP, self, EVENT_CALLBACK(Synth_onRequestMidiChannelStop), sizeof(int));
    Event_unsubscribe(EVENT_REQUEST_SEQUENCER_START, self, EVENT_CALLBACK(Synth_onRequestSequencerStart), sizeof(SequencerRequest));
    Event_unsubscribe(EVENT_REQUEST_SEQUENCER_STOP, self, EVENT_CALLBACK(Synth_onRequestSequencerStop), 0);
    Event_unsubscribe(EVENT_SEQUENCER_STARTED, self, EVENT_CALLBACK(Synth_onSequencerStarted), sizeof(SequencerRequest));

    Event_unsubscribe(EVENT_SEQUENCER_CALLBACK, self, EVENT_CALLBACK(Synth_onSequencerCallback), 0);

    fluid_sequencer_unregister_client(self->sequencer, self->callbackId);
    delete_fluid_sequencer(self->sequencer);
    delete_fluid_audio_driver(self->audioDriver);
    delete_fluid_synth(self->fluidSynth);
    delete_fluid_settings(self->settings);
    StringMap_free(&self->synthInstrumentMap);
    sfree((void**)&self->synthInstrumentListString);
    sfree((void**)&self->synthInstruments);
    sfree((void**)pself);
}


static void Synth_onProcessFrame(Synth* self, void* sender, float* deltaTime) {
    (void)sender; (void)deltaTime;
    if (self->isSequencerRunning) {
        unsigned int sequencerTick = fluid_sequencer_get_tick(self->sequencer);
        float progress = ((float)sequencerTick - (float)self->sequencerStartTimeTicks) / ((float)self->sequencerEndTimeTicks - (float)self->sequencerStartTimeTicks);
        float fullProgress = (1.0f - self->sequencerInitialProgressFraction) * progress + self->sequencerInitialProgressFraction;
        Event_post(self, EVENT_SEQUENCER_PROGRESS, &fullProgress, sizeof(fullProgress));
    }
}


static void Synth_onQueryResult(Synth* self, void* sender, QueryResult* queryResult) {
    (void)sender;
    if (!strcmp(queryResult->key, REQUEST_KEY_CHANGE_SYNTH_INSTRUMENT)) {
        const char* synthInstrumentName = queryResult->value;
        Synth_setSynthProgram(self, synthInstrumentName, self->lastRequestedSynthInstrumentChangeChannel);
    }
}


static void Synth_onRequestChangeSynthInstrument(Synth* self, void* sender, SynthProgramChange* synthProgramChange) {
    (void)sender;
    Synth_setSynthProgram(self, synthProgramChange->name, synthProgramChange->iChannel);
}


static void Synth_onRequestChangeSynthInstrumentQuery(Synth* self, void* sender, int* iChannel) {
    (void)sender;
    self->lastRequestedSynthInstrumentChangeChannel = *iChannel;
    QueryRequest queryRequest = {
        .key = REQUEST_KEY_CHANGE_SYNTH_INSTRUMENT,
        .prompt = "Set instrument:",
        .items = self->synthInstrumentListString,
    };
    Event_post(self, EVENT_REQUEST_QUERY, &queryRequest, sizeof(queryRequest));
}


static void Synth_onRequestMidiMessagePlay(Synth* self, void* sender, MidiMessage* midiMessage) {
    (void)sender;
    if (midiMessage->type == MIDI_MESSAGE_TYPE_NOTEON) {
        fluid_synth_noteon(self->fluidSynth, midiMessage->channel, midiMessage->pitch, midiMessage->velocity);
    } else if (midiMessage->type == MIDI_MESSAGE_TYPE_NOTEOFF) {
        fluid_synth_noteoff(self->fluidSynth, midiMessage->channel, midiMessage->pitch);
    } else {
        Log_fatal("Unknown MIDI message type %d", midiMessage->type);
    }
}


static void Synth_onRequestMidiChannelStop(Synth* self, void* sender, int* iChannel) {
    (void)sender;
    fluid_synth_all_notes_off(self->fluidSynth, *iChannel);
}


static void Synth_onRequestSequencerStart(Synth* self, void* sender, SequencerRequest* sequencerRequest) {
    (void)sender;
    self->sequencerStartTimeTicks = fluid_sequencer_get_tick(self->sequencer);
    float sequencerTimeScale = fluid_sequencer_get_time_scale(self->sequencer);

    self->sequencerInitialProgressFraction = sequencerRequest->timestampStart / sequencerRequest->timestampEnd;

    for (size_t i = 0; i < sequencerRequest->nMidiMessages; i++) {
        float timestampSeconds = sequencerRequest->midiMessages[i].timestampSeconds;
        Log_assert(timestampSeconds >= sequencerRequest->timestampStart, "Midi messages before start time");

        fluid_event_t* event = new_fluid_event();
        fluid_event_set_source(event, -1);
        fluid_event_set_dest(event, self->synthSequencerId);
        int type = sequencerRequest->midiMessages[i].type;
        int channel = sequencerRequest->midiMessages[i].channel;
        int pitch = sequencerRequest->midiMessages[i].pitch;
        int velocity = sequencerRequest->midiMessages[i].velocity;
        unsigned int midiMessageTimeTicks = self->sequencerStartTimeTicks + sequencerTimeScale * (timestampSeconds - sequencerRequest->timestampStart);

        if (type == MIDI_MESSAGE_TYPE_NOTEON) {
            fluid_event_noteon(event, channel, pitch, velocity);
        } else if (type == MIDI_MESSAGE_TYPE_NOTEOFF) {
            fluid_event_noteoff(event, channel, pitch);

            // Prevent off and on messages on the same sequencer tick for back-to-back notes
            if (midiMessageTimeTicks > 0) {
                midiMessageTimeTicks--;
            }
        } else {
            Log_fatal("Unknown MIDI message type %d", type);
        }

        fluid_sequencer_send_at(self->sequencer, event, midiMessageTimeTicks, true);
        delete_fluid_event(event);
    }

    self->sequencerEndTimeTicks = self->sequencerStartTimeTicks + sequencerTimeScale * (sequencerRequest->timestampEnd - sequencerRequest->timestampStart);
    fluid_event_t* event = new_fluid_event();
    fluid_event_set_source(event, -1);
    fluid_event_set_dest(event, self->callbackId);
    fluid_event_timer(event, NULL);
    fluid_sequencer_send_at(self->sequencer, event, self->sequencerEndTimeTicks, true);
    delete_fluid_event(event);

    Event_post(self, EVENT_SEQUENCER_STARTED, sequencerRequest, sizeof(*sequencerRequest));
}


static void Synth_onSequencerStarted(Synth* self, void* sender, SequencerRequest* sequencerRequest) {
    (void)sender; (void)sequencerRequest;
    self->isSequencerRunning = true;
}


static void Synth_onRequestSequencerStop(Synth* self, void* sender, void* unused) {
    (void)sender; (void)unused;
    Event_post(self, EVENT_SEQUENCER_CALLBACK, NULL, 0);
}


static void Synth_sequencerCallback(unsigned int time, fluid_event_t* event, fluid_sequencer_t* sequencer, void* data) {
    (void)time; (void)data;
    if (fluid_event_get_type(event) != FLUID_SEQ_TIMER) {
        return;
    }
    Event_post(sequencer, EVENT_SEQUENCER_CALLBACK, NULL, 0);
}


static void Synth_setSynthProgram(Synth* self, const char* synthProgramName, int iChannel) {
    if (StringMap_containsItem(self->synthInstrumentMap, synthProgramName)) {
        SynthInstrument* synthInstrument = StringMap_getItem(self->synthInstrumentMap, synthProgramName);
        fluid_synth_program_select(
            self->fluidSynth,
            iChannel,
            synthInstrument->iSoundFont,
            synthInstrument->iBank,
            synthInstrument->iProgram
        );

        SynthProgramChange synthProgramChange = {0};
        snprintf(synthProgramChange.name, SYNTH_PROGRAM_CHANGE_NAME_BUFFER_SIZE, "%s", synthProgramName);
        synthProgramChange.iChannel = iChannel;
        Event_post(self, EVENT_SYNTH_INSTRUMENT_CHANGED, &synthProgramChange, sizeof(synthProgramChange));
    } else {
        Log_warning("Ignoring invalid synth instrument name '%s'", synthProgramName);
    }
}


static void Synth_onSequencerCallback(Synth* self, void* sender, void* unused) {
    (void)sender; (void)unused;
    fluid_sequencer_remove_events(self->sequencer, -1, -1, -1);
    for (int iChannel = 0; iChannel < SYNTH_MIDI_CHANNELS; iChannel++) {
        fluid_synth_all_notes_off(self->fluidSynth, iChannel);
    }
    self->isSequencerRunning = false;
    Event_post(self, EVENT_SEQUENCER_STOPPED, NULL, 0);
}
