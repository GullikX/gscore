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

#include "score.h"

#include "common/constants/fluidmidi.h"
#include "common/constants/keysignatures.h"
#include "common/score/fileformatschema.h"
#include "common/score/xmlconstants.h"
#include "common/structs/blockinstance.h"
#include "common/structs/color.h"
#include "common/structs/midimessage.h"
#include "common/structs/note.h"
#include "common/structs/queryrequest.h"
#include "common/structs/queryresult.h"
#include "common/structs/sequencerrequest.h"
#include "common/structs/synthprogramchange.h"
#include "common/util/alloc.h"
#include "common/util/colors.h"
#include "common/util/hashset.h"
#include "common/util/log.h"
#include "common/util/stringmap.h"
#include "common/util/stringset.h"
#include "common/util/version.h"
#include "config/config.h"
#include "events/events.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlschemas.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char* const REQUEST_KEY_CHANGE_KEY_SIGNATURE = "REQUEST_KEY_CHANGE_KEY_SIGNATURE";
static const char* const REQUEST_KEY_CHANGE_ACTIVE_BLOCK = "REQUEST_KEY_CHANGE_ACTIVE_BLOCK";
static const char* const REQUEST_KEY_CHANGE_BLOCK_COLOR = "REQUEST_KEY_CHANGE_BLOCK_COLOR";
static const char* const REQUEST_KEY_CHANGE_TRACK_VELOCITY = "REQUEST_KEY_CHANGE_TRACK_VELOCITY";
static const char* const REQUEST_KEY_RENAME_BLOCK = "REQUEST_KEY_RENAME_BLOCK";
static const char* const REQUEST_KEY_SET_TEMPO_BPM = "REQUEST_KEY_SET_TEMPO_BPM";

enum {
    XML_BUFFER_SIZE = 1024,
    HEX_COLOR_BUFFER_SIZE = 7,
    SECONDS_PER_MINUTE = 60,
};

static const float BLOCK_NEW_COLOR_VARIATION = 0.2f;

struct Score {
    char* filename;
    xmlDocPtr xmlDoc;
    xmlNodePtr nodeScore;
    xmlNodePtr nodeBlockDefs;
    xmlNodePtr nodeTracks;
    xmlNodePtr nodeBlockDefCurrent;
    xmlNodePtr nodeBlockDefPrev;
    xmlSchemaParserCtxtPtr parserContext;
    xmlSchemaPtr schema;
    xmlSchemaValidCtxtPtr validationContext;
    char* blockListString;
    int iLastQueriedTrack;
};


static void Score_onQueryResult(Score* self, void* sender, QueryResult* queryResult);
static void Score_onSynthInstrumentChanged(Score* self, void* sender, SynthProgramChange* synthProgramChange);
static bool Score_fileExists(Score* self);
static void Score_createNew(Score* self);
static void Score_loadFromFile(Score* self);
static bool Score_isXmlDocValid(Score* self);
static xmlNodePtr Score_createNewBlockDef(Score* self, const char* blockName);
static xmlNodePtr Score_createNewTrack(Score* self);
static void Score_requestBlockDefNotes(Score* self, xmlNodePtr nodeBlockDef);
static bool Score_blockDefExists(Score* self, const char* blockName);
static xmlNodePtr Score_getNodeBlockDefByName(Score* self, const char* blockName);
static void Score_updateBlockListString(Score* self);
static void Score_setActiveBlockdef(Score* self, xmlNodePtr nodeBlockDef);
static float Score_getBlockDurationSeconds(Score* self);
static MidiMessage* Score_getMidiMessagesFromBlockdef(Score* self, xmlNodePtr nodeBlockDef, float startTimeFraction, bool ignoreNoteOff, int iChannel, size_t* outAmount);
static xmlNodePtr findXmlNodeByName(xmlNodePtr node, const char* const nodeName);
static Note createNoteFromXmlNodes(xmlNodePtr nodeMessageOn, xmlNodePtr nodeMessageOff);
static int getXmlNodeChildCount(xmlNodePtr node);
static xmlNodePtr getXmlChildNodeByIndex(xmlNodePtr node, int index);
static bool hasXmlNodeProperty(xmlNodePtr node, const char* propertyKey);
static const char* getXmlNodePropertyString(xmlNodePtr node, const char* propertyKey);
static int getXmlNodePropertyInt(xmlNodePtr node, const char* propertyKey);
static float getXmlNodePropertyFloat(xmlNodePtr node, const char* propertyKey);
static void setXmlNodePropertyString(xmlNodePtr node, const char* propertyKey, const char* propertyValue);
static void setXmlNodePropertyInt(xmlNodePtr node, const char* propertyKey, int propertyValue);
static void setXmlNodePropertyFloat(xmlNodePtr node, const char* propertyKey, float propertyValue);
static void unsetXmlNodeProperty(xmlNodePtr node, const char* propertyKey);
static xmlNodePtr addMidiMessageToBlockDef(xmlNodePtr nodeBlockDef, int messageType, int pitch, float time, float velocity);
static xmlDocPtr pruneScore(xmlDocPtr xmlDoc);
static int getKeySignatureIndex(const char* keySignatureName);
static void sortMidiMessages(MidiMessage* midiMessages, size_t nMidiMessages);
static int compareMidiMessages(const void* midiMessage, const void* midiMessageOther);
static int compareMidiMessageNodes(xmlNodePtr midiMessageNode, xmlNodePtr midiMessageNodeOther);


Score* Score_new(const char* const filename) {
    Score* self = ecalloc(1, sizeof(*self));
    self->filename = estrdup(filename);

    // Pretty print XML
    xmlKeepBlanksDefault(0);

    self->parserContext = xmlSchemaNewMemParserCtxt(FILE_FORMAT_SCHEMA, strlen(FILE_FORMAT_SCHEMA));
    Log_assert(self->parserContext, "Could not create XSD-schema parsing context");
    self->schema = xmlSchemaParse(self->parserContext);
    Log_assert(self->schema, "Could not parse XSD-schema");
    self->validationContext = xmlSchemaNewValidCtxt(self->schema);
    Log_assert(self->validationContext, "Could not create XSD-schema validation context");

    if (Score_fileExists(self)) {
        Score_loadFromFile(self);
    } else {
        Score_createNew(self);
    }

    self->nodeBlockDefCurrent = findXmlNodeByName(self->nodeBlockDefs, XMLNODE_BLOCKDEF);
    Score_updateBlockListString(self);

    Event_subscribe(EVENT_QUERY_RESULT, self, EVENT_CALLBACK(Score_onQueryResult), sizeof(QueryResult));
    Event_subscribe(EVENT_SYNTH_INSTRUMENT_CHANGED, self, EVENT_CALLBACK(Score_onSynthInstrumentChanged), sizeof(SynthProgramChange));

    return self;
}


void Score_free(Score** pself) {
    Score* self = *pself;

    Event_unsubscribe(EVENT_QUERY_RESULT, self, EVENT_CALLBACK(Score_onQueryResult), sizeof(QueryResult));
    Event_unsubscribe(EVENT_SYNTH_INSTRUMENT_CHANGED, self, EVENT_CALLBACK(Score_onSynthInstrumentChanged), sizeof(SynthProgramChange));

    if (self->parserContext) {
        xmlSchemaFreeParserCtxt(self->parserContext);
        self->parserContext = NULL;
    }
    if (self->schema) {
        xmlSchemaFree(self->schema);
        self->schema = NULL;
    }
    if (self->validationContext) {
        xmlSchemaFreeValidCtxt(self->validationContext);
        self->validationContext = NULL;
    }
    if (self->xmlDoc) {
        xmlFreeDoc(self->xmlDoc);
        self->xmlDoc = NULL;
    }

    sfree((void**)&self->filename);
    sfree((void**)&self->blockListString);
    sfree((void**)pself);
}


void Score_addNote(Score* self, int pitch, float time, float duration, float velocity) {
    Score_removeNotes(self, pitch, time, time + duration);

    xmlNodePtr nodeMessageOn = addMidiMessageToBlockDef(self->nodeBlockDefCurrent, MIDI_MESSAGE_TYPE_NOTEON, pitch, time, velocity);
    xmlNodePtr nodeMessageOff = addMidiMessageToBlockDef(self->nodeBlockDefCurrent, MIDI_MESSAGE_TYPE_NOTEOFF, pitch, time + duration, 0.0);

    Note note = createNoteFromXmlNodes(nodeMessageOn, nodeMessageOff);
    Event_post(self, EVENT_NOTE_ADDED, &note, sizeof(note));
}


void Score_removeNotes(Score* self, int pitch, float timeStart, float timeEnd) {
    size_t childCount = 0;
    for (xmlNodePtr nodeMessage = self->nodeBlockDefCurrent->children; nodeMessage; nodeMessage = nodeMessage->next) {
        childCount++;
    }
    xmlNodePtr* nodesOnToRemove = ecalloc(childCount, sizeof(xmlNodePtr));
    xmlNodePtr* nodesOffToRemove = ecalloc(childCount, sizeof(xmlNodePtr));

    size_t nNotesToRemove = 0;

    for (xmlNodePtr nodeMessage = self->nodeBlockDefCurrent->children; nodeMessage; nodeMessage = nodeMessage->next) {
        if (nodeMessage->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_MESSAGE, (char*)nodeMessage->name)) {
            int midiMessageType = getXmlNodePropertyInt(nodeMessage, XMLATTRIB_TYPE);
            if (midiMessageType != MIDI_MESSAGE_TYPE_NOTEON) {
                continue;
            }
            int midiMessagePitch = getXmlNodePropertyInt(nodeMessage, XMLATTRIB_PITCH);
            if (midiMessagePitch != pitch) {
                continue;
            }
            float midiMessageTime = getXmlNodePropertyFloat(nodeMessage, XMLATTRIB_TIME);
            if (midiMessageTime >= timeEnd) {
                continue;
            }
            for (xmlNodePtr nodeMessageOther = nodeMessage; nodeMessageOther; nodeMessageOther = nodeMessageOther->next) {
                if (nodeMessageOther->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_MESSAGE, (char*)nodeMessageOther->name)) {
                    int midiMessageTypeOther = getXmlNodePropertyInt(nodeMessageOther, XMLATTRIB_TYPE);
                    if (midiMessageTypeOther != MIDI_MESSAGE_TYPE_NOTEOFF) {
                        continue;
                    }
                    int midiMessagePitchOther = getXmlNodePropertyInt(nodeMessageOther, XMLATTRIB_PITCH);
                    if (midiMessagePitchOther != pitch) {
                        continue;
                    }
                    float midiMessageTimeOther = getXmlNodePropertyFloat(nodeMessageOther, XMLATTRIB_TIME);
                    if (midiMessageTimeOther <= timeStart) {
                        break;
                    }

                    nodesOnToRemove[nNotesToRemove] = nodeMessage;
                    nodesOffToRemove[nNotesToRemove] = nodeMessageOther;
                    nNotesToRemove++;

                    break;
                }
            }
        }
    }

    for (size_t i = 0; i < nNotesToRemove; i++) {
        xmlNodePtr nodeMessageOn = nodesOnToRemove[i];
        xmlNodePtr nodeMessageOff = nodesOffToRemove[i];

        Note note = createNoteFromXmlNodes(nodeMessageOn, nodeMessageOff);
        Event_post(self, EVENT_NOTE_REMOVED, &note, sizeof(note));

        xmlUnlinkNode(nodeMessageOn);
        xmlUnlinkNode(nodeMessageOff);
        xmlFreeNode(nodeMessageOn);
        xmlFreeNode(nodeMessageOff);
    }

    sfree((void**)&nodesOnToRemove);
    sfree((void**)&nodesOffToRemove);
}


void Score_saveToFile(Score* self) {
    Log_info("Saving score as '%s'...", self->filename);

    Log_assert(Score_isXmlDocValid(self), "XML document is invalid prior to saving?");

    xmlDocPtr xmlDocPruned = pruneScore(self->xmlDoc);

    int bytesWritten = xmlSaveFormatFileEnc(self->filename, xmlDocPruned, XML_ENCODING, 1);
    if (bytesWritten >= 0) {
        Log_info("Score saved (%d bytes).", bytesWritten);
    } else {
        Log_error("Failed to write score to '%s'", self->filename);
    }

    xmlFreeDoc(xmlDocPruned);
}


void Score_playCurrentBlockDef(Score* self, int iChannel, float startTimeFraction, bool ignoreNoteOff) {
    size_t nMidiMessages = 0;
    MidiMessage* midiMessages = Score_getMidiMessagesFromBlockdef(self, self->nodeBlockDefCurrent, startTimeFraction, ignoreNoteOff, iChannel, &nMidiMessages);

    for (size_t i = 0; i < nMidiMessages; i++) {
        midiMessages[i].velocity *= BLOCK_VELOCITY_DEFAULT * TRACK_VELOCITY_DEFAULT;
    }

    sortMidiMessages(midiMessages, nMidiMessages);

    float blockDurationSeconds = Score_getBlockDurationSeconds(self);
    float startTimeSeconds = startTimeFraction * blockDurationSeconds;

    SequencerRequest sequencerRequest = {
        .timestampStart = startTimeSeconds,
        .timestampEnd = blockDurationSeconds,
        .nMidiMessages = nMidiMessages,
        .midiMessages = midiMessages,
    };

    Event_post(self, EVENT_REQUEST_SEQUENCER_START, &sequencerRequest, sizeof(sequencerRequest));

    sfree((void**)&midiMessages);
}


void Score_playEntireScore(Score* self, int iTimeSlotStart) {
    float blockDurationSeconds = Score_getBlockDurationSeconds(self);
    float startTimeSeconds = blockDurationSeconds * iTimeSlotStart;
    float endTimeSeconds = blockDurationSeconds * SCORE_LENGTH_MAX;

    StringMap* midiMessagesPerBlock = StringMap_new(64);
    StringMap* nMidiMessagesPerBlock = StringMap_new(64);

    for (xmlNodePtr nodeBlockDef = self->nodeBlockDefs->children; nodeBlockDef; nodeBlockDef = nodeBlockDef->next) {
        if (nodeBlockDef->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCKDEF, (char*)nodeBlockDef->name)) {
            size_t nMidiMessages = 0;
            MidiMessage* midiMessages = Score_getMidiMessagesFromBlockdef(self, nodeBlockDef, 0.0, false, 0, &nMidiMessages);
            uintptr_t nMidiMessagesUintptr = nMidiMessages;

            const char* blockDefName = getXmlNodePropertyString(nodeBlockDef, XMLATTRIB_NAME);
            StringMap_addItem(midiMessagesPerBlock, blockDefName, midiMessages);
            StringMap_addItem(nMidiMessagesPerBlock, blockDefName, (void*)nMidiMessagesUintptr);
            sfree((void**)&blockDefName);
        }
    }

    HashSet* allMidiMessages = HashSet_new(sizeof(MidiMessage));

    int iTrack = 0;
    for (xmlNodePtr nodeTrack = self->nodeTracks->children; nodeTrack; nodeTrack = nodeTrack->next) {
        if (nodeTrack->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_TRACK, (char*)nodeTrack->name)) {
            float trackVelocity = getXmlNodePropertyFloat(nodeTrack, XMLATTRIB_VELOCITY);
            bool trackIgnoreNoteOff = getXmlNodePropertyInt(nodeTrack, XMLATTRIB_IGNORENOTEOFF);
            int iTimeSlot = 0;
            for (xmlNodePtr nodeBlock = nodeTrack->children; nodeBlock; nodeBlock = nodeBlock->next) {
                if (nodeBlock->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCK, (char*)nodeBlock->name)) {
                    if (hasXmlNodeProperty(nodeBlock, XMLATTRIB_NAME) && iTimeSlot >= iTimeSlotStart) {
                        float blockVelocity = getXmlNodePropertyFloat(nodeBlock, XMLATTRIB_VELOCITY);
                        const char* blockName = getXmlNodePropertyString(nodeBlock, XMLATTRIB_NAME);
                        MidiMessage* blockMidiMessages = StringMap_getItem(midiMessagesPerBlock, blockName);
                        uintptr_t nBlockMidiMessages = (uintptr_t)StringMap_getItem(nMidiMessagesPerBlock, blockName);

                        for (uintptr_t i = 0; i < nBlockMidiMessages; i++) {
                            MidiMessage midiMessage = {
                                .type = blockMidiMessages[i].type,
                                .channel = iTrack + 1, // Channel 0 is editview synth channel
                                .pitch = blockMidiMessages[i].pitch,
                                .velocity = blockMidiMessages[i].velocity * blockVelocity * trackVelocity,
                                .timestampSeconds = iTimeSlot * blockDurationSeconds + blockMidiMessages[i].timestampSeconds,
                            };

                            if (midiMessage.type == MIDI_MESSAGE_TYPE_NOTEON) {
                                HashSet_addItem(allMidiMessages, &midiMessage);
                            } else if (midiMessage.type == MIDI_MESSAGE_TYPE_NOTEOFF) {
                                if (!trackIgnoreNoteOff) {
                                    HashSet_addItem(allMidiMessages, &midiMessage);
                                }
                            } else {
                                Log_fatal("Unknown MIDI message type %d", midiMessage.type);
                            }
                        }

                        sfree((void**)&blockName);
                    }
                    iTimeSlot++;
                }
            }
            iTrack++;
        }
    }

    for (const char* blockName = StringMap_iterateInit(midiMessagesPerBlock); blockName; blockName = StringMap_iterateNext(midiMessagesPerBlock, blockName)) {
        MidiMessage* blockMidiMessages = StringMap_getItem(midiMessagesPerBlock, blockName);
        sfree((void**)&blockMidiMessages);
    }
    StringMap_free(&midiMessagesPerBlock);
    StringMap_free(&nMidiMessagesPerBlock);

    size_t nMidiMessages = HashSet_countItems(allMidiMessages);

    MidiMessage* midiMessages = ecalloc(nMidiMessages, sizeof(MidiMessage));
    size_t iMidiMessage = 0;
    for (MidiMessage* midiMessage = HashSet_iterateInit(allMidiMessages); midiMessage; midiMessage = HashSet_iterateNext(allMidiMessages, midiMessage)) {
        midiMessages[iMidiMessage] = *midiMessage;
        iMidiMessage++;
    }

    sortMidiMessages(midiMessages, nMidiMessages);

    HashSet_free(&allMidiMessages);

    SequencerRequest sequencerRequest = {
        .timestampStart = startTimeSeconds,
        .timestampEnd = endTimeSeconds,
        .nMidiMessages = nMidiMessages,
        .midiMessages = midiMessages,
    };

    Event_post(self, EVENT_REQUEST_SEQUENCER_START, &sequencerRequest, sizeof(sequencerRequest));

    sfree((void**)&midiMessages);
}


void Score_requestCurrentBlockDefNotes(Score* self) {
    Score_requestBlockDefNotes(self, self->nodeBlockDefCurrent);
}


void Score_requestPrevBlockDefNotes(Score* self) {
    Score_requestBlockDefNotes(self, self->nodeBlockDefPrev);
}


void Score_requestCurrentBlockDefColor(Score* self) {
    const char* hexColor = getXmlNodePropertyString(self->nodeBlockDefCurrent, XMLATTRIB_COLOR);
    Color color = parseHexColor(hexColor);
    Event_post(self, EVENT_BLOCK_COLOR_CHANGED, &color, sizeof(color));
    sfree((void**)&hexColor);
}


void Score_requestKeySignature(Score* self) {
    const char* keySignatureName = getXmlNodePropertyString(self->nodeScore, XMLATTRIB_KEYSIGNATURE);
    int iKeySignature = getKeySignatureIndex(keySignatureName);
    Log_assert(iKeySignature >= 0, "Invalid key signature name: '%s'", keySignatureName);
    Event_post(self, EVENT_KEY_SIGNATURE_CHANGED, &iKeySignature, sizeof(iKeySignature));
    sfree((void**)&keySignatureName);
}


void Score_requestBlockInstances(Score* self) {
    int iTrack = 0;
    for (xmlNodePtr nodeTrack = self->nodeTracks->children; nodeTrack; nodeTrack = nodeTrack->next) {
        if (nodeTrack->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_TRACK, (char*)nodeTrack->name)) {
            int iTimeSlot = 0;
            for (xmlNodePtr nodeBlock = nodeTrack->children; nodeBlock; nodeBlock = nodeBlock->next) {
                if (nodeBlock->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCK, (char*)nodeBlock->name)) {
                    if (hasXmlNodeProperty(nodeBlock, XMLATTRIB_NAME)) {
                        const char* blockName = getXmlNodePropertyString(nodeBlock, XMLATTRIB_NAME);
                        xmlNodePtr nodeBlockDef = Score_getNodeBlockDefByName(self, blockName);
                        sfree((void**)&blockName);

                        const char* hexColor = getXmlNodePropertyString(nodeBlockDef, XMLATTRIB_COLOR);
                        Color color = parseHexColor(hexColor);
                        sfree((void**)&hexColor);

                        BlockInstance blockInstance = {
                            .iTrack = iTrack,
                            .iTimeSlot = iTimeSlot,
                            .color = color,
                        };

                        Event_post(self, EVENT_BLOCK_INSTANCE_ADDED, &blockInstance, sizeof(blockInstance));
                    }
                    iTimeSlot++;
                }
            }
            iTrack++;
        }
    }
}


void Score_requestSynthPrograms(Score* self) {
    int iTrack = 0;
    for (xmlNodePtr nodeTrack = self->nodeTracks->children; nodeTrack; nodeTrack = nodeTrack->next) {
        if (nodeTrack->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_TRACK, (char*)nodeTrack->name)) {
            if (hasXmlNodeProperty(nodeTrack, XMLATTRIB_PROGRAM)) {
                const char* synthProgramName = getXmlNodePropertyString(nodeTrack, XMLATTRIB_PROGRAM);

                SynthProgramChange synthProgramChange = {0};
                snprintf(synthProgramChange.name, SYNTH_PROGRAM_CHANGE_NAME_BUFFER_SIZE, "%s", synthProgramName);
                synthProgramChange.iChannel = iTrack + 1;
                Event_post(self, EVENT_REQUEST_CHANGE_SYNTH_INSTRUMENT, &synthProgramChange, sizeof(synthProgramChange));

                sfree((void**)&synthProgramName);
            };
            iTrack++;
        }
    }

}


void Score_stopPlaying(Score* self) {
    Event_post(self, EVENT_REQUEST_SEQUENCER_STOP, NULL, 0);
}


void Score_changeKeySignature(Score* self) {
    QueryRequest queryRequest = {
        .key = REQUEST_KEY_CHANGE_KEY_SIGNATURE,
        .prompt = "Set key signature:",
        .items = KEY_SIGNATURE_LIST_STRING,
    };
    Event_post(self, EVENT_REQUEST_QUERY, &queryRequest, sizeof(queryRequest));
}


void Score_changeActiveBlockDef(Score* self) {
    QueryRequest queryRequest = {
        .key = REQUEST_KEY_CHANGE_ACTIVE_BLOCK,
        .prompt = "Select block:",
        .items = self->blockListString,
    };
    Event_post(self, EVENT_REQUEST_QUERY, &queryRequest, sizeof(queryRequest));
}


void Score_toggleActiveBlockDef(Score* self) {
    Score_setActiveBlockdef(self, self->nodeBlockDefPrev);
}


void Score_pickBlockDef(Score* self, int iTrack, int iTimeSlot) {
    if (iTrack < getXmlNodeChildCount(self->nodeTracks)) {
        xmlNodePtr nodeTrack = getXmlChildNodeByIndex(self->nodeTracks, iTrack);
        if (iTimeSlot < getXmlNodeChildCount(nodeTrack)) {
            xmlNodePtr nodeBlock = getXmlChildNodeByIndex(nodeTrack, iTimeSlot);

            if (hasXmlNodeProperty(nodeBlock, XMLATTRIB_NAME)) {
                const char* blockName = getXmlNodePropertyString(nodeBlock, XMLATTRIB_NAME);
                xmlNodePtr nodeBlockDef = Score_getNodeBlockDefByName(self, blockName);
                sfree((void**)&blockName);

                Score_setActiveBlockdef(self, nodeBlockDef);
            }

            if (hasXmlNodeProperty(nodeTrack, XMLATTRIB_PROGRAM)) {
                const char* synthProgramName = getXmlNodePropertyString(nodeTrack, XMLATTRIB_PROGRAM);

                SynthProgramChange synthProgramChange = {0};
                snprintf(synthProgramChange.name, SYNTH_PROGRAM_CHANGE_NAME_BUFFER_SIZE, "%s", synthProgramName);
                sfree((void**)&synthProgramName);

                synthProgramChange.iChannel = 0; // edit view
                Event_post(self, EVENT_REQUEST_CHANGE_SYNTH_INSTRUMENT, &synthProgramChange, sizeof(synthProgramChange));
            }

            if (hasXmlNodeProperty(nodeTrack, XMLATTRIB_IGNORENOTEOFF)) {
                int ignoreNoteOff = getXmlNodePropertyInt(nodeTrack, XMLATTRIB_IGNORENOTEOFF);
                Event_post(self, EVENT_REQUEST_IGNORE_NOTEOFF, &ignoreNoteOff, sizeof(ignoreNoteOff));
            }
        }
    }
}


void Score_changeCurrentBlockDefColor(Score* self) {
    const char* currentHexColor = getXmlNodePropertyString(self->nodeBlockDefCurrent, XMLATTRIB_COLOR);
    QueryRequest queryRequest = {
        .key = REQUEST_KEY_CHANGE_BLOCK_COLOR,
        .prompt = "Set block color:",
        .items = currentHexColor,
    };
    Event_post(self, EVENT_REQUEST_QUERY, &queryRequest, sizeof(queryRequest));
    sfree((void**)&currentHexColor);
}


void Score_renameCurrentBlockDef(Score* self) {
    const char* currentBlockName = getXmlNodePropertyString(self->nodeBlockDefCurrent, XMLATTRIB_NAME);
    QueryRequest queryRequest = {
        .key = REQUEST_KEY_RENAME_BLOCK,
        .prompt = "Rename block:",
        .items = currentBlockName,
    };
    Event_post(self, EVENT_REQUEST_QUERY, &queryRequest, sizeof(queryRequest));
    sfree((void**)&currentBlockName);
}


void Score_setTempoBpm(Score* self) {
    const char* currentTempoBpm = getXmlNodePropertyString(self->nodeScore, XMLATTRIB_TEMPO);
    QueryRequest queryRequest = {
        .key = REQUEST_KEY_SET_TEMPO_BPM,
        .prompt = "Set tempo (BPM):",
        .items = currentTempoBpm,
    };
    Event_post(self, EVENT_REQUEST_QUERY, &queryRequest, sizeof(queryRequest));
    sfree((void**)&currentTempoBpm);
}


void Score_changeTrackVelocity(Score* self, int iTrack) {
    float velocityPrev = TRACK_VELOCITY_DEFAULT;

    if (iTrack < getXmlNodeChildCount(self->nodeTracks)) {
        xmlNodePtr nodeTrack = getXmlChildNodeByIndex(self->nodeTracks, iTrack);
        if (hasXmlNodeProperty(nodeTrack, XMLATTRIB_VELOCITY)) {
            velocityPrev = getXmlNodePropertyFloat(nodeTrack, XMLATTRIB_VELOCITY);
        }
    }

    char buffer[XML_BUFFER_SIZE] = {0};
    snprintf(buffer, XML_BUFFER_SIZE, "%f", velocityPrev);

    QueryRequest queryRequest = {
        .key = REQUEST_KEY_CHANGE_TRACK_VELOCITY,
        .prompt = "Set track velocity:",
        .items = buffer,
    };

    self->iLastQueriedTrack = iTrack;
    Event_post(self, EVENT_REQUEST_QUERY, &queryRequest, sizeof(queryRequest));
}


void Score_addBlockInstance(Score* self, int iTrack, int iTimeSlot) {
    Score_removeBlockInstance(self, iTrack, iTimeSlot);

    while (getXmlNodeChildCount(self->nodeTracks) < iTrack + 1) {
        Score_createNewTrack(self);
    }

    xmlNodePtr nodeTrack = getXmlChildNodeByIndex(self->nodeTracks, iTrack);

    while (getXmlNodeChildCount(nodeTrack) < iTimeSlot + 1) {
        xmlNewChild(nodeTrack, NULL, BAD_CAST XMLNODE_BLOCK, NULL);
    }

    xmlNodePtr nodeBlock = getXmlChildNodeByIndex(nodeTrack, iTimeSlot);

    const char* blockName = getXmlNodePropertyString(self->nodeBlockDefCurrent, XMLATTRIB_NAME);
    setXmlNodePropertyString(nodeBlock, XMLATTRIB_NAME, blockName);
    sfree((void**)&blockName);

    setXmlNodePropertyFloat(nodeBlock, XMLATTRIB_VELOCITY, BLOCK_VELOCITY_DEFAULT);

    const char* hexColor = getXmlNodePropertyString(self->nodeBlockDefCurrent, XMLATTRIB_COLOR);
    Color color = parseHexColor(hexColor);
    sfree((void**)&hexColor);

    BlockInstance blockInstance = {
        .iTrack = iTrack,
        .iTimeSlot = iTimeSlot,
        .color = color,
    };

    Event_post(self, EVENT_BLOCK_INSTANCE_ADDED, &blockInstance, sizeof(blockInstance));
}


void Score_removeBlockInstance(Score* self, int iTrack, int iTimeSlot) {
    if (iTrack < getXmlNodeChildCount(self->nodeTracks)) {
        xmlNodePtr nodeTrack = getXmlChildNodeByIndex(self->nodeTracks, iTrack);
        if (iTimeSlot < getXmlNodeChildCount(nodeTrack)) {
            xmlNodePtr nodeBlock = getXmlChildNodeByIndex(nodeTrack, iTimeSlot);

            if (hasXmlNodeProperty(nodeBlock, XMLATTRIB_NAME) || hasXmlNodeProperty(nodeBlock, XMLATTRIB_VELOCITY)) {
                unsetXmlNodeProperty(nodeBlock, XMLATTRIB_NAME);
                unsetXmlNodeProperty(nodeBlock, XMLATTRIB_VELOCITY);

                const char* hexColor = getXmlNodePropertyString(self->nodeBlockDefCurrent, XMLATTRIB_COLOR);
                Color color = parseHexColor(hexColor);
                sfree((void**)&hexColor);

                BlockInstance blockInstance = {
                    .iTrack = iTrack,
                    .iTimeSlot = iTimeSlot,
                    .color = color,
                };

                Event_post(self, EVENT_BLOCK_INSTANCE_REMOVED, &blockInstance, sizeof(blockInstance));
            }
        }
    }
}


void Score_toggleIgnoreNoteOff(Score* self, int iTrack) {
    if (iTrack < getXmlNodeChildCount(self->nodeTracks)) {
        xmlNodePtr nodeTrack = getXmlChildNodeByIndex(self->nodeTracks, iTrack);

        bool ignoreNoteOffPrev = false;

        if (hasXmlNodeProperty(nodeTrack, XMLATTRIB_IGNORENOTEOFF)) {
            ignoreNoteOffPrev = getXmlNodePropertyInt(nodeTrack, XMLATTRIB_IGNORENOTEOFF);
        }

        bool ignoreNoteOff = !ignoreNoteOffPrev;
        setXmlNodePropertyInt(nodeTrack, XMLATTRIB_IGNORENOTEOFF, ignoreNoteOff);
        Log_info("Ignore note off for track %d: %s", iTrack, ignoreNoteOff ? "true" : "false");
    }
}


static void Score_onQueryResult(Score* self, void* sender, QueryResult* queryResult) {
    (void)sender;
    if (!strcmp(queryResult->key, REQUEST_KEY_CHANGE_KEY_SIGNATURE)) {
        const char* keySignatureName = queryResult->value;
        int iKeySignature = getKeySignatureIndex(keySignatureName);
        if (iKeySignature >= 0) {
            setXmlNodePropertyString(self->nodeScore, XMLATTRIB_KEYSIGNATURE, keySignatureName);
            Event_post(self, EVENT_KEY_SIGNATURE_CHANGED, &iKeySignature, sizeof(iKeySignature));
        } else {
            Log_warning("Invalid key signature name: '%s'", keySignatureName);
        }
    } else if (!strcmp(queryResult->key, REQUEST_KEY_CHANGE_ACTIVE_BLOCK)) {
        const char* blockName = queryResult->value;
        xmlNodePtr nodeBlockDef = NULL;
        if (Score_blockDefExists(self, blockName)) {
            nodeBlockDef = Score_getNodeBlockDefByName(self, blockName);
        } else {
            Log_info("Creating new block: '%s'", blockName);
            nodeBlockDef = Score_createNewBlockDef(self, blockName);
            Score_updateBlockListString(self);
        }
        Score_setActiveBlockdef(self, nodeBlockDef);
    } else if (!strcmp(queryResult->key, REQUEST_KEY_CHANGE_BLOCK_COLOR)) {
        const char* hexColor = queryResult->value;
        Color color = parseHexColor(hexColor);
        setXmlNodePropertyString(self->nodeBlockDefCurrent, XMLATTRIB_COLOR, hexColor);
        Event_post(self, EVENT_BLOCK_COLOR_CHANGED, &color, sizeof(color));
    } else if (!strcmp(queryResult->key, REQUEST_KEY_RENAME_BLOCK)) {
        const char* blockNamePrev = getXmlNodePropertyString(self->nodeBlockDefCurrent, XMLATTRIB_NAME);

        const char* blockNameNew = queryResult->value;
        if (strcmp(blockNameNew, blockNamePrev)) {
            setXmlNodePropertyString(self->nodeBlockDefCurrent, XMLATTRIB_NAME, blockNameNew);
            Score_updateBlockListString(self);

            for (xmlNodePtr nodeTrack = self->nodeTracks->children; nodeTrack; nodeTrack = nodeTrack->next) {
                if (nodeTrack->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_TRACK, (char*)nodeTrack->name)) {
                    for (xmlNodePtr nodeBlock = nodeTrack->children; nodeBlock; nodeBlock = nodeBlock->next) {
                        if (nodeBlock->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCK, (char*)nodeBlock->name)) {
                            if (hasXmlNodeProperty(nodeBlock, XMLATTRIB_NAME)) {
                                const char* blockName = getXmlNodePropertyString(nodeBlock, XMLATTRIB_NAME);
                                if (!strcmp(blockName, blockNamePrev)) {
                                    setXmlNodePropertyString(nodeBlock, XMLATTRIB_NAME, blockNameNew);
                                }
                                sfree((void**)&blockName);
                            }
                        }
                    }
                }
            }
        }
        sfree((void**)&blockNamePrev);
    } else if (!strcmp(queryResult->key, REQUEST_KEY_SET_TEMPO_BPM)) {
        const char* tempoBpmString = queryResult->value;
        int tempoBpm = atoi(tempoBpmString);
        if (tempoBpm) {
            setXmlNodePropertyString(self->nodeScore, XMLATTRIB_TEMPO, tempoBpmString);
        } else {
            Log_warning("Invalid tempo BPM '%s'", tempoBpmString);
        }
    } else if (!strcmp(queryResult->key, REQUEST_KEY_CHANGE_TRACK_VELOCITY)) {
        const char* trackVelocityString = queryResult->value;
        float trackVelocity = atof(trackVelocityString);

        if (self->iLastQueriedTrack < getXmlNodeChildCount(self->nodeTracks)) {
            xmlNodePtr nodeTrack = getXmlChildNodeByIndex(self->nodeTracks, self->iLastQueriedTrack);
            setXmlNodePropertyFloat(nodeTrack, XMLATTRIB_VELOCITY, trackVelocity);
        }
    }
}


static void Score_onSynthInstrumentChanged(Score* self, void* sender, SynthProgramChange* synthProgramChange) {
    if (synthProgramChange->iChannel > 0) {
        int iTrack = synthProgramChange->iChannel - 1;

        while (getXmlNodeChildCount(self->nodeTracks) < iTrack + 1) {
            Score_createNewTrack(self);
        }

        xmlNodePtr nodeTrack = getXmlChildNodeByIndex(self->nodeTracks, iTrack);
        setXmlNodePropertyString(nodeTrack, XMLATTRIB_PROGRAM, synthProgramChange->name);
    }
}


static bool Score_fileExists(Score* self) {
    return access(self->filename, F_OK) != -1;
}


static void Score_createNew(Score* self) {
    Log_assert(!self->xmlDoc, "An XML document is already loaded");

    self->xmlDoc = xmlNewDoc(BAD_CAST XML_VERSION);
    xmlNodePtr nodeGscore = xmlNewNode(NULL, BAD_CAST XMLNODE_GSCORE);
    xmlDocSetRootElement(self->xmlDoc, nodeGscore);
    setXmlNodePropertyString(nodeGscore, XMLATTRIB_VERSION, VERSION);

    self->nodeScore = xmlNewChild(nodeGscore, NULL, BAD_CAST XMLNODE_SCORE, NULL);
    {
        char buffer[XML_BUFFER_SIZE] = {0};
        snprintf(buffer, XML_BUFFER_SIZE, "%d", TEMPO_BPM_DEFAULT);
        setXmlNodePropertyString(self->nodeScore, XMLATTRIB_TEMPO, buffer);
    }
    {
        char buffer[XML_BUFFER_SIZE] = {0};
        snprintf(buffer, XML_BUFFER_SIZE, "%d", N_BEATS_PER_MEASURE_DEFAULT);
        setXmlNodePropertyString(self->nodeScore, XMLATTRIB_BEATSPERMEASURE, buffer);
    }
    setXmlNodePropertyString(self->nodeScore, XMLATTRIB_KEYSIGNATURE, KEY_SIGNATURE_NAMES[KEY_SIGNATURE_DEFAULT]);

    xmlNodePtr nodeMetadata = xmlNewNode(NULL, BAD_CAST XMLNODE_METADATA);
    xmlAddChild(self->nodeScore, xmlCopyNodeList(nodeMetadata));
    xmlFreeNode(nodeMetadata);

    self->nodeBlockDefs = xmlNewChild(self->nodeScore, NULL, BAD_CAST XMLNODE_BLOCKDEFS, NULL);
    Score_createNewBlockDef(self, BLOCK_NAME_DEFAULT);

    self->nodeTracks = xmlNewChild(self->nodeScore, NULL, BAD_CAST XMLNODE_TRACKS, NULL);
    Score_createNewTrack(self);

    Log_assert(Score_isXmlDocValid(self), "Newly created XML document is invalid?");
}


static void Score_loadFromFile(Score* self) {
    Log_assert(!self->xmlDoc, "An XML document is already loaded");
    self->xmlDoc = xmlReadFile(self->filename, NULL, 0);
    Log_assert(Score_isXmlDocValid(self), "File '%s' could not be read", self->filename);

    self->nodeScore = findXmlNodeByName(xmlDocGetRootElement(self->xmlDoc), XMLNODE_SCORE);
    Log_assert(self->nodeScore, "%s xml node not found", XMLNODE_SCORE);

    self->nodeBlockDefs = findXmlNodeByName(self->nodeScore, XMLNODE_BLOCKDEFS);
    Log_assert(self->nodeBlockDefs, "%s xml node not found", XMLNODE_BLOCKDEFS);

    self->nodeTracks = findXmlNodeByName(self->nodeScore, XMLNODE_TRACKS);
    Log_assert(self->nodeTracks, "%s xml node not found", XMLNODE_TRACKS);
}


static bool Score_isXmlDocValid(Score* self) {
    return self->xmlDoc && xmlSchemaValidateDoc(self->validationContext, self->xmlDoc) == 0;
}


static xmlNodePtr Score_createNewBlockDef(Score* self, const char* blockName) {
    Log_assert(!Score_blockDefExists(self, blockName), "Block with name '%s' already exists!", blockName);

    xmlNodePtr nodeBlockDef = xmlNewChild(self->nodeBlockDefs, NULL, BAD_CAST XMLNODE_BLOCKDEF, NULL);
    setXmlNodePropertyString(nodeBlockDef, XMLATTRIB_NAME, blockName);

    Color blockColorLerpTarget = generateColorFromString(blockName);
    float blockColorLerpWeight = BLOCK_NEW_COLOR_VARIATION;

    if (!strcmp(blockName, BLOCK_NAME_DEFAULT)) {
        blockColorLerpWeight = 0.0;
    }

    Color blockColor = lerpColor(BLOCK_COLOR_DEFAULT, blockColorLerpTarget, blockColorLerpWeight);

    char hexColor[HEX_COLOR_BUFFER_SIZE] = {0};
    unsigned int r = 255.0f * blockColor.r;
    unsigned int g = 255.0f * blockColor.g;
    unsigned int b = 255.0f * blockColor.b;
    snprintf(hexColor, HEX_COLOR_BUFFER_SIZE, "%02X%02X%02X", r, g, b);
    setXmlNodePropertyString(nodeBlockDef, XMLATTRIB_COLOR, hexColor);

    return nodeBlockDef;

}


static xmlNodePtr Score_createNewTrack(Score* self) {
    xmlNodePtr nodeTrack = xmlNewChild(self->nodeTracks, NULL, BAD_CAST XMLNODE_TRACK, NULL);

    char buffer[XML_BUFFER_SIZE] = {0};
    setXmlNodePropertyString(nodeTrack, XMLATTRIB_PROGRAM, SYNTH_PROGRAM_NAME_DEFAULT);
    snprintf(buffer, XML_BUFFER_SIZE, "%f", TRACK_VELOCITY_DEFAULT);
    setXmlNodePropertyString(nodeTrack, XMLATTRIB_VELOCITY, buffer);
    snprintf(buffer, XML_BUFFER_SIZE, "%d", 0);
    setXmlNodePropertyString(nodeTrack, XMLATTRIB_IGNORENOTEOFF, buffer);

    return nodeTrack;
}


static void Score_requestBlockDefNotes(Score* self, xmlNodePtr nodeBlockDef) {
    if (!nodeBlockDef) {
        return;
    }

    for (xmlNodePtr nodeMessage = nodeBlockDef->children; nodeMessage; nodeMessage = nodeMessage->next) {
        if (nodeMessage->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_MESSAGE, (char*)nodeMessage->name)) {
            int midiMessageType = getXmlNodePropertyInt(nodeMessage, XMLATTRIB_TYPE);
            if (midiMessageType != MIDI_MESSAGE_TYPE_NOTEON) {
                continue;
            }
            int midiMessagePitch = getXmlNodePropertyInt(nodeMessage, XMLATTRIB_PITCH);
            float midiMessageTime = getXmlNodePropertyFloat(nodeMessage, XMLATTRIB_TIME);
            for (xmlNodePtr nodeMessageOther = nodeMessage; nodeMessageOther; nodeMessageOther = nodeMessageOther->next) {
                if (nodeMessageOther->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_MESSAGE, (char*)nodeMessageOther->name)) {
                    int midiMessageTypeOther = getXmlNodePropertyInt(nodeMessageOther, XMLATTRIB_TYPE);
                    if (midiMessageTypeOther != MIDI_MESSAGE_TYPE_NOTEOFF) {
                        continue;
                    }
                    int midiMessagePitchOther = getXmlNodePropertyInt(nodeMessageOther, XMLATTRIB_PITCH);
                    if (midiMessagePitchOther != midiMessagePitch) {
                        continue;
                    }
                    float midiMessageTimeOther = getXmlNodePropertyFloat(nodeMessageOther, XMLATTRIB_TIME);
                    Log_assert(midiMessageTimeOther > midiMessageTime, "Midi messages unordered? (expected %f > %f)", midiMessageTimeOther, midiMessageTime);

                    Note note = createNoteFromXmlNodes(nodeMessage, nodeMessageOther);
                    Event_post(self, EVENT_NOTE_ADDED, &note, sizeof(note));

                    break;
                }
            }
        }
    }
}


static bool Score_blockDefExists(Score* self, const char* blockName) {
    for (xmlNodePtr nodeBlockDef = self->nodeBlockDefs->children; nodeBlockDef; nodeBlockDef = nodeBlockDef->next) {
        if (nodeBlockDef->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCKDEF, (char*)nodeBlockDef->name)) {
            const char* xmlBlockDefName = getXmlNodePropertyString(nodeBlockDef, XMLATTRIB_NAME);
            bool matches = !strcmp(xmlBlockDefName, blockName);
            sfree((void**)&xmlBlockDefName);
            if (matches) {
                return true;
            }
        }
    }
    return false;
}


static xmlNodePtr Score_getNodeBlockDefByName(Score* self, const char* blockName) {
    for (xmlNodePtr nodeBlockDef = self->nodeBlockDefs->children; nodeBlockDef; nodeBlockDef = nodeBlockDef->next) {
        if (nodeBlockDef->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCKDEF, (char*)nodeBlockDef->name)) {
            const char* xmlBlockDefName = getXmlNodePropertyString(nodeBlockDef, XMLATTRIB_NAME);
            bool matches = !strcmp(xmlBlockDefName, blockName);
            sfree((void**)&xmlBlockDefName);
            if (matches) {
                return nodeBlockDef;
            }
        }
    }
    Log_fatal("No such blockdef: '%s'", blockName);
    return NULL;
}


static void Score_updateBlockListString(Score* self) {
    if (self->blockListString) {
        sfree((void**)&self->blockListString);
    }

    size_t blockListStringLength = 1;  // space for null-terminator

    for (xmlNodePtr nodeBlockDef = self->nodeBlockDefs->children; nodeBlockDef; nodeBlockDef = nodeBlockDef->next) {
        if (nodeBlockDef->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCKDEF, (char*)nodeBlockDef->name)) {
            const char* xmlBlockDefName = getXmlNodePropertyString(nodeBlockDef, XMLATTRIB_NAME);
            blockListStringLength += strlen(xmlBlockDefName);
            blockListStringLength += strlen("\n");
            sfree((void**)&xmlBlockDefName);
        }
    }

    self->blockListString = ecalloc(blockListStringLength, sizeof(char));

    for (xmlNodePtr nodeBlockDef = self->nodeBlockDefs->children; nodeBlockDef; nodeBlockDef = nodeBlockDef->next) {
        if (nodeBlockDef->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCKDEF, (char*)nodeBlockDef->name)) {
            const char* xmlBlockDefName = getXmlNodePropertyString(nodeBlockDef, XMLATTRIB_NAME);
            strcat(self->blockListString, xmlBlockDefName);
            strcat(self->blockListString, "\n");
            sfree((void**)&xmlBlockDefName);
        }
    }
}


static void Score_setActiveBlockdef(Score* self, xmlNodePtr nodeBlockDef) {
    if (!nodeBlockDef || nodeBlockDef == self->nodeBlockDefCurrent) {
        return;
    }

    self->nodeBlockDefPrev = self->nodeBlockDefCurrent;
    self->nodeBlockDefCurrent = nodeBlockDef;

    const char* blockDefName = getXmlNodePropertyString(nodeBlockDef, XMLATTRIB_NAME);
    Log_info("Active block set: '%s'", blockDefName);
    sfree((void**)&blockDefName);

    const char* hexColor = getXmlNodePropertyString(self->nodeBlockDefCurrent, XMLATTRIB_COLOR);
    Color color = parseHexColor(hexColor);

    Event_post(self, EVENT_ACTIVE_BLOCK_CHANGED, &color, sizeof(color));

    sfree((void**)&hexColor);
}


static MidiMessage* Score_getMidiMessagesFromBlockdef(Score* self, xmlNodePtr nodeBlockDef, float startTimeFraction, bool ignoreNoteOff, int iChannel, size_t* outAmount) {
    float blockDurationSeconds = Score_getBlockDurationSeconds(self);

    size_t nMidiMessages = 0;
    for (xmlNodePtr nodeMessage = nodeBlockDef->children; nodeMessage; nodeMessage = nodeMessage->next) {
        if (nodeMessage->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_MESSAGE, (char*)nodeMessage->name)) {
            if (ignoreNoteOff && getXmlNodePropertyInt(nodeMessage, XMLATTRIB_TYPE) == MIDI_MESSAGE_TYPE_NOTEOFF) {
                continue;
            }

            float midiMessageTime = getXmlNodePropertyFloat(nodeMessage, XMLATTRIB_TIME);
            if (midiMessageTime >= startTimeFraction) {
                nMidiMessages++;
            }
        }
    }
    MidiMessage* midiMessages = ecalloc(nMidiMessages, sizeof(MidiMessage));
    size_t i = 0;

    for (xmlNodePtr nodeMessage = nodeBlockDef->children; nodeMessage; nodeMessage = nodeMessage->next) {
        if (nodeMessage->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_MESSAGE, (char*)nodeMessage->name)) {
            if (ignoreNoteOff && getXmlNodePropertyInt(nodeMessage, XMLATTRIB_TYPE) == MIDI_MESSAGE_TYPE_NOTEOFF) {
                continue;
            }

            float midiMessageTime = getXmlNodePropertyFloat(nodeMessage, XMLATTRIB_TIME);
            if (midiMessageTime >= startTimeFraction) {
                Log_assert(i < nMidiMessages, "index too large? (expected %zu < %zu)", i, nMidiMessages);
                midiMessages[i].type = getXmlNodePropertyInt(nodeMessage, XMLATTRIB_TYPE);
                midiMessages[i].channel = iChannel;
                midiMessages[i].pitch = getXmlNodePropertyInt(nodeMessage, XMLATTRIB_PITCH);
                midiMessages[i].velocity = (float)MIDI_MESSAGE_VELOCITY_MAX * getXmlNodePropertyFloat(nodeMessage, XMLATTRIB_VELOCITY);
                midiMessages[i].timestampSeconds = midiMessageTime * blockDurationSeconds;

                i++;
            }
        }
    }

    sortMidiMessages(midiMessages, nMidiMessages);

    *outAmount = nMidiMessages;
    return midiMessages;
}


static float Score_getBlockDurationSeconds(Score* self) {
    int tempoBpm = getXmlNodePropertyInt(self->nodeScore, XMLATTRIB_TEMPO);
    Log_assert(tempoBpm > 0, "Expected tempo BPM to be larger than 0, but was %d", tempoBpm);

    int nBeatsPerMeasure = getXmlNodePropertyInt(self->nodeScore, XMLATTRIB_BEATSPERMEASURE);
    Log_assert(nBeatsPerMeasure > 0, "Expected beats per measure to be larger than 0, but was %d", nBeatsPerMeasure);

    return (float)(N_BLOCK_MEASURES * nBeatsPerMeasure * SECONDS_PER_MINUTE) / (float)tempoBpm;
}


static xmlNodePtr findXmlNodeByName(xmlNodePtr node, const char* const nodeName) {
    while (node) {
        if (node->type == XML_ELEMENT_NODE && !strcmp((const char*)node->name, nodeName)) {
            return node;
        }

        xmlNodePtr result = findXmlNodeByName(node->children, nodeName);
        if (result) {
            return result;
        }

        node = node->next;
    }
    return NULL;
}


static Note createNoteFromXmlNodes(xmlNodePtr nodeMessageOn, xmlNodePtr nodeMessageOff) {
    int messageOnType = getXmlNodePropertyInt(nodeMessageOn, XMLATTRIB_TYPE);
    Log_assert(messageOnType == MIDI_MESSAGE_TYPE_NOTEON, "Expected midi message to be of type %d, but was %d", MIDI_MESSAGE_TYPE_NOTEON, messageOnType);

    int messageOffType = getXmlNodePropertyInt(nodeMessageOff, XMLATTRIB_TYPE);
    Log_assert(messageOffType == MIDI_MESSAGE_TYPE_NOTEOFF, "Expected midi message to be of type %d, but was %d", MIDI_MESSAGE_TYPE_NOTEOFF, messageOffType);

    int messagePitch = getXmlNodePropertyInt(nodeMessageOn, XMLATTRIB_PITCH);
    float messageTimeOn = getXmlNodePropertyFloat(nodeMessageOn, XMLATTRIB_TIME);
    float messageTimeOff = getXmlNodePropertyFloat(nodeMessageOff, XMLATTRIB_TIME);
    float messageVelocity = getXmlNodePropertyFloat(nodeMessageOn, XMLATTRIB_VELOCITY);

    Note note = {
        messagePitch,
        messageTimeOn,
        messageTimeOff - messageTimeOn,
        messageVelocity,
    };

    return note;
}


static int getXmlNodeChildCount(xmlNodePtr node) {
    int childCount = 0;

    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE) {
            childCount++;
        }
    }

    return childCount;
}


static xmlNodePtr getXmlChildNodeByIndex(xmlNodePtr node, int iChild) {
    int i = 0;

    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE) {
            if (i == iChild) {
                return child;
            }
            i++;
        }
    }

    Log_fatal("Failed to get child xml node at index %zu", iChild);
    return NULL;
}


static bool hasXmlNodeProperty(xmlNodePtr node, const char* propertyKey) {
    char* propertyValue = (char*)xmlGetProp(node, BAD_CAST propertyKey);
    if (propertyValue) {
        xmlFree(propertyValue);
        return true;
    } else {
        return false;
    }
}


static const char* getXmlNodePropertyString(xmlNodePtr node, const char* propertyKey) {
    Log_assert(node, "Cannot get property '%s' from null node", propertyKey);
    char* propertyValue = (char*)xmlGetProp(node, BAD_CAST propertyKey);
    Log_assert(propertyValue, "Failed to get value of XML property '%s'", propertyKey);
    Log_assert(strlen(propertyValue) > 0, "Value of XML property '%s' is empty", propertyKey);
    const char* propertyValueString = estrdup(propertyValue);
    xmlFree(propertyValue);
    return propertyValueString;
}


static int getXmlNodePropertyInt(xmlNodePtr node, const char* propertyKey) {
    const char* propertyValueString = getXmlNodePropertyString(node, propertyKey);
    int propertyValueInt = atoi(propertyValueString);
    sfree((void**)&propertyValueString);
    return propertyValueInt;
}


static float getXmlNodePropertyFloat(xmlNodePtr node, const char* propertyKey) {
    const char* propertyValueString = getXmlNodePropertyString(node, propertyKey);
    float propertyValueFloat = atof(propertyValueString);
    sfree((void**)&propertyValueString);
    return propertyValueFloat;
}


static void setXmlNodePropertyString(xmlNodePtr node, const char* propertyKey, const char* propertyValue) {
    Log_assert(node, "Cannot get property '%s'='%s' on null node", propertyKey, propertyValue);
    xmlSetProp(node, BAD_CAST propertyKey, BAD_CAST propertyValue);
}


static void setXmlNodePropertyInt(xmlNodePtr node, const char* propertyKey, int propertyValue) {
    char buffer[XML_BUFFER_SIZE] = {0};
    snprintf(buffer, XML_BUFFER_SIZE, "%d", propertyValue);
    setXmlNodePropertyString(node, propertyKey, buffer);
}


static void setXmlNodePropertyFloat(xmlNodePtr node, const char* propertyKey, float propertyValue) {
    char buffer[XML_BUFFER_SIZE] = {0};
    snprintf(buffer, XML_BUFFER_SIZE, "%f", propertyValue);
    setXmlNodePropertyString(node, propertyKey, buffer);
}


static void unsetXmlNodeProperty(xmlNodePtr node, const char* propertyKey) {
    if (hasXmlNodeProperty(node, propertyKey)) {
        xmlUnsetProp(node, BAD_CAST propertyKey);
    } else {
        Log_warning("Tried to remove non-existing property '%s' from xml node %p", propertyKey, (void*)node);
    }
}


static xmlNodePtr addMidiMessageToBlockDef(xmlNodePtr nodeBlockDef, int messageType, int pitch, float time, float velocity) {
    xmlNodePtr nodeMessage = xmlNewNode(NULL, BAD_CAST XMLNODE_MESSAGE);
    setXmlNodePropertyFloat(nodeMessage, XMLATTRIB_TIME, time);
    setXmlNodePropertyInt(nodeMessage, XMLATTRIB_TYPE, messageType);
    setXmlNodePropertyInt(nodeMessage, XMLATTRIB_PITCH, pitch);
    setXmlNodePropertyFloat(nodeMessage, XMLATTRIB_VELOCITY, velocity);

    xmlNodePtr nodeMessageOtherFirst = nodeBlockDef->children;
    xmlNodePtr nodeMessageOtherLast = nodeBlockDef->last;

    if (!nodeMessageOtherFirst || !nodeMessageOtherLast) {
        Log_assert(!nodeMessageOtherFirst && !nodeMessageOtherLast, "Inconsistent xml children");
        xmlAddChild(nodeBlockDef, nodeMessage);
    } else if (compareMidiMessageNodes(nodeMessage, nodeMessageOtherFirst) < 0) {
        xmlAddPrevSibling(nodeMessageOtherFirst, nodeMessage);
    } else if (compareMidiMessageNodes(nodeMessage, nodeMessageOtherLast) > 0) {
        xmlAddNextSibling(nodeMessageOtherLast, nodeMessage);
    } else {
        bool nodeAdded = false;
        for (xmlNodePtr nodeMessageOther = nodeMessageOtherFirst; nodeMessageOther != nodeMessageOtherLast; nodeMessageOther = nodeMessageOther->next) {
            if (compareMidiMessageNodes(nodeMessage, nodeMessageOther) > 0 && compareMidiMessageNodes(nodeMessage, nodeMessageOther->next) < 0) {
                xmlAddNextSibling(nodeMessageOther, nodeMessage);
                nodeAdded = true;
                break;
            }
        }
        Log_assert(nodeAdded, "Failed to add midi message of type %d at time %f", messageType, time);
    }

    return nodeMessage;
}


static xmlDocPtr pruneScore(xmlDocPtr xmlDoc) {
    xmlDocPtr xmlDocPruned = xmlCopyDoc(xmlDoc, 1);
    Log_assert(xmlDocPruned, "Error when copying XML document");

    xmlNodePtr nodeScore = findXmlNodeByName(xmlDocGetRootElement(xmlDocPruned), XMLNODE_SCORE);
    Log_assert(nodeScore, "%s xml node not found", XMLNODE_SCORE);

    xmlNodePtr nodeBlockDefs = findXmlNodeByName(nodeScore, XMLNODE_BLOCKDEFS);
    Log_assert(nodeBlockDefs, "%s xml node not found", XMLNODE_BLOCKDEFS);

    xmlNodePtr nodeTracks = findXmlNodeByName(nodeScore, XMLNODE_TRACKS);
    Log_assert(nodeTracks, "%s xml node not found", XMLNODE_TRACKS);

    /* Prune empty tracks */ {
        HashSet* emptyTracks = HashSet_new(sizeof(xmlNodePtr));

        for (xmlNodePtr nodeTrack = nodeTracks->children; nodeTrack; nodeTrack = nodeTrack->next) {
            if (nodeTrack->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_TRACK, (char*)nodeTrack->name)) {
                HashSet_addItem(emptyTracks, &nodeTrack);
            }
        }

        for (xmlNodePtr nodeTrack = nodeTracks->children; nodeTrack; nodeTrack = nodeTrack->next) {
            if (nodeTrack->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_TRACK, (char*)nodeTrack->name)) {
                for (xmlNodePtr nodeBlock = nodeTrack->children; nodeBlock; nodeBlock = nodeBlock->next) {
                    if (nodeBlock->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCK, (char*)nodeBlock->name)) {
                        if (hasXmlNodeProperty(nodeBlock, XMLATTRIB_NAME)) {
                            if (HashSet_containsItem(emptyTracks, &nodeTrack)) {
                                HashSet_removeItem(emptyTracks, &nodeTrack);
                            }
                        }
                    }
                }
            }
        }

        size_t nEmptyTracks = HashSet_countItems(emptyTracks);
        if (nEmptyTracks > 0) {
            Log_warning("Discarding %zu empty instrument track(s)", nEmptyTracks);
            for (xmlNodePtr* emptyTrack = HashSet_iterateInit(emptyTracks); emptyTrack; emptyTrack = HashSet_iterateNext(emptyTracks, emptyTrack)) {
                xmlUnlinkNode(*emptyTrack);
                xmlFreeNode(*emptyTrack);
            }
        }

        HashSet_free(&emptyTracks);
    }

    /* Prune end of tracks */ {
        HashSet* nullBlockInstances = HashSet_new(sizeof(xmlNodePtr));

        for (xmlNodePtr nodeTrack = nodeTracks->children; nodeTrack; nodeTrack = nodeTrack->next) {
            if (nodeTrack->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_TRACK, (char*)nodeTrack->name)) {
                for (xmlNodePtr nodeBlock = nodeTrack->last; nodeBlock; nodeBlock = nodeBlock->prev) {
                    if (nodeBlock->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCK, (char*)nodeBlock->name)) {
                        if (hasXmlNodeProperty(nodeBlock, XMLATTRIB_NAME)) {
                            break;
                        } else {
                            HashSet_addItem(nullBlockInstances, &nodeBlock);
                        }
                    }
                }
            }
        }

        for (xmlNodePtr* nullBlock = HashSet_iterateInit(nullBlockInstances); nullBlock; nullBlock = HashSet_iterateNext(nullBlockInstances, nullBlock)) {
            xmlUnlinkNode(*nullBlock);
            xmlFreeNode(*nullBlock);
        }

        HashSet_free(&nullBlockInstances);
    }

    /* Prune unused block defs */ {
        StringSet* usedBlockDefNames = StringSet_new(64);
        for (xmlNodePtr nodeTrack = nodeTracks->children; nodeTrack; nodeTrack = nodeTrack->next) {
            if (nodeTrack->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_TRACK, (char*)nodeTrack->name)) {
                for (xmlNodePtr nodeBlock = nodeTrack->children; nodeBlock; nodeBlock = nodeBlock->next) {
                    if (nodeBlock->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCK, (char*)nodeBlock->name)) {
                        if (hasXmlNodeProperty(nodeBlock, XMLATTRIB_NAME)) {
                            const char* blockName = getXmlNodePropertyString(nodeBlock, XMLATTRIB_NAME);
                            StringSet_addItem(usedBlockDefNames, blockName);
                            sfree((void**)&blockName);
                        }
                    }
                }
            }
        }

        HashSet* unusedBlockDefs = HashSet_new(sizeof(xmlNodePtr));
        for (xmlNodePtr nodeBlockDef = nodeBlockDefs->children; nodeBlockDef; nodeBlockDef = nodeBlockDef->next) {
            if (nodeBlockDef->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCKDEF, (char*)nodeBlockDef->name)) {
                const char* blockName = getXmlNodePropertyString(nodeBlockDef, XMLATTRIB_NAME);
                if (!StringSet_containsItem(usedBlockDefNames, blockName)) {
                    HashSet_addItem(unusedBlockDefs, &nodeBlockDef);
                }
                sfree((void**)&blockName);
            }
        }

        size_t nUnusedBlockDefs = HashSet_countItems(unusedBlockDefs);
        if (nUnusedBlockDefs > 0) {
            Log_warning("Discarding %zu unused block definition(s):", nUnusedBlockDefs);
            for (xmlNodePtr* unusedBlockDef = HashSet_iterateInit(unusedBlockDefs); unusedBlockDef; unusedBlockDef = HashSet_iterateNext(unusedBlockDefs, unusedBlockDef)) {
                const char* blockName = getXmlNodePropertyString(*unusedBlockDef, XMLATTRIB_NAME);
                size_t nMidiMessages = 0;
                for (xmlNodePtr nodeMessage = (*unusedBlockDef)->children; nodeMessage; nodeMessage = nodeMessage->next) {
                    if (nodeMessage->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_MESSAGE, (char*)nodeMessage->name)) {
                        nMidiMessages++;
                    }
                }
                Log_warning("    %s (%zu midi messages)", blockName, nMidiMessages);
                sfree((void**)&blockName);

                xmlUnlinkNode(*unusedBlockDef);
                xmlFreeNode(*unusedBlockDef);
            }
        }

        StringSet_free(&usedBlockDefNames);
        HashSet_free(&unusedBlockDefs);
    }

    return xmlDocPruned;
}


static int getKeySignatureIndex(const char* keySignatureName) {
    for (int iKeySignature = 0; iKeySignature < KEY_SIGNATURE_COUNT; iKeySignature++) {
        if (!strcmp(keySignatureName, KEY_SIGNATURE_NAMES[iKeySignature])) {
            return iKeySignature;
        }
    }
    return -1;
}


static void sortMidiMessages(MidiMessage* midiMessages, size_t nMidiMessages) {
    qsort(midiMessages, nMidiMessages, sizeof(MidiMessage), compareMidiMessages);
}


static int compareMidiMessages(const void* midiMessage, const void* midiMessageOther) {
    MidiMessage* m = (MidiMessage*)midiMessage;
    MidiMessage* mOther = (MidiMessage*)midiMessageOther;

    if (m->timestampSeconds > mOther->timestampSeconds) {
        return 1;
    } else if (m->timestampSeconds < mOther->timestampSeconds) {
        return -1;
    } else if (m->type == MIDI_MESSAGE_TYPE_NOTEON && mOther->type == MIDI_MESSAGE_TYPE_NOTEOFF) {
        return 1;
    } else if (m->type == MIDI_MESSAGE_TYPE_NOTEOFF && mOther->type == MIDI_MESSAGE_TYPE_NOTEON) {
        return -1;
    } else if (m->pitch > mOther->pitch) {
        return 1;
    } else if (m->pitch < mOther->pitch) {
        return -1;
    } else {
        return 0;
    }
}


static int compareMidiMessageNodes(xmlNodePtr midiMessageNode, xmlNodePtr midiMessageNodeOther) {
    MidiMessage midiMessage = {
        .type = getXmlNodePropertyInt(midiMessageNode, XMLATTRIB_TYPE),
        .pitch = getXmlNodePropertyInt(midiMessageNode, XMLATTRIB_PITCH),
        .velocity = getXmlNodePropertyInt(midiMessageNode, XMLATTRIB_VELOCITY),
        .timestampSeconds = getXmlNodePropertyFloat(midiMessageNode, XMLATTRIB_TIME),
    };

    MidiMessage midiMessageOther = {
        .type = getXmlNodePropertyInt(midiMessageNodeOther, XMLATTRIB_TYPE),
        .pitch = getXmlNodePropertyInt(midiMessageNodeOther, XMLATTRIB_PITCH),
        .velocity = getXmlNodePropertyInt(midiMessageNodeOther, XMLATTRIB_VELOCITY),
        .timestampSeconds = getXmlNodePropertyFloat(midiMessageNodeOther, XMLATTRIB_TIME),
    };

    return compareMidiMessages(&midiMessage, &midiMessageOther);
}
