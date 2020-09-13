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

Score* Score_new(Synth* synth) {
    Score* self = ecalloc(1, sizeof(*self));
    self->tempo = TEMPO_BPM;

    self->blocks[0] = Block_new(BLOCK_NAME_DEFAULT, COLOR_BLOCK_DEFAULT);
    self->nBlocks = 1;

    self->tracks[0] = Track_new(Synth_getDefaultProgramName(synth), DEFAULT_VELOCITY, false);
    self->nTracks = 1;

    self->scoreLength = SCORE_LENGTH_DEFAULT;
    self->keySignature = KEY_SIGNATURE_DEFAULT;

    return self;
}


Score* Score_free(Score* self) {
    for (int iTrack = 0; iTrack < self->nTracks; iTrack++) {
        self->tracks[iTrack] = Track_free(self->tracks[iTrack]);
    }
    for (int iBlock = 0; iBlock < self->nBlocks; iBlock++) {
        self->blocks[iBlock] = Block_free(self->blocks[iBlock]);
    }
    free(self);
    return NULL;
}


Score* Score_readFromFile(const char* const filename, Synth* synth) {
    Score* self = ecalloc(1, sizeof(*self));
    xmlDocPtr doc = xmlReadFile(filename, NULL, 0);
    xmlNode* nodeRoot = xmlDocGetRootElement(doc);
    if (!nodeRoot) {
        die("Failed to parse input file '%s'", filename);
    }
    if (strcmp(XMLNODE_SCORE, (char*)nodeRoot->name)) {
        die("Unexpected node name '%s', expected '%s'", (char*)nodeRoot->name, XMLNODE_SCORE);
    }

    self->tempo = TEMPO_BPM;
    {
        char* tempoProp = (char*)xmlGetProp(nodeRoot, BAD_CAST XMLATTRIB_TEMPO);
        if (tempoProp) {
            self->tempo = atoi(tempoProp);
        }
        xmlFree(tempoProp);
    }

    self->keySignature = KEY_SIGNATURE_DEFAULT;
    {
        char* keySignatureProp = (char*)xmlGetProp(nodeRoot, BAD_CAST XMLATTRIB_KEYSIGNATURE);
        if (keySignatureProp) {
            Score_setKeySignatureByName(self, keySignatureProp);
        }
        xmlFree(keySignatureProp);
    }

    /* Read block definitions */
    for (xmlNode* node = nodeRoot->children; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCKDEFS, (char*)node->name)) {
            self->nBlocks = 0;
            for (xmlNode* nodeBlockDef = node->children; nodeBlockDef; nodeBlockDef = nodeBlockDef->next) {
                if (nodeBlockDef->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCKDEF, (char*)nodeBlockDef->name)) {
                    if (self->nBlocks >= MAX_BLOCKS) die("To many blocks (max is %d)", MAX_BLOCKS);

                    {
                        const char* blockName = NULL;
                        char* blockNameProp = (char*)xmlGetProp(nodeBlockDef, BAD_CAST XMLATTRIB_NAME);
                        if (blockNameProp) {
                            blockName = blockNameProp;
                        }

                        const char* blockColor = COLOR_BLOCK_DEFAULT;
                        char* blockColorProp = (char*)xmlGetProp(nodeBlockDef, BAD_CAST XMLATTRIB_COLOR);
                        if (blockColorProp) {
                            blockColor = blockColorProp;
                        }

                        self->blocks[self->nBlocks] = Block_new(blockName, blockColor);

                        xmlFree(blockNameProp);
                        xmlFree(blockColorProp);
                    }


                    for (xmlNode* nodeMessage = nodeBlockDef->children; nodeMessage; nodeMessage = nodeMessage->next) {
                        if (nodeMessage->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_MESSAGE, (char*)nodeMessage->name)) {
                            int type = 0;
                            {
                                char* typeProp = (char*)xmlGetProp(nodeMessage, BAD_CAST XMLATTRIB_TYPE);
                                if (typeProp) {
                                    type = atoi(typeProp);
                                }
                                xmlFree(typeProp);
                            }

                            float time = -1.0f;
                            {
                                char* timeProp = (char*)xmlGetProp(nodeMessage, BAD_CAST XMLATTRIB_TIME);
                                if (timeProp) {
                                    time = atof(timeProp);
                                }
                                xmlFree(timeProp);
                            }

                            int pitch = -1;
                            {
                                char* pitchProp = (char*)xmlGetProp(nodeMessage, BAD_CAST XMLATTRIB_PITCH);
                                if (pitchProp) {
                                    pitch = atoi(pitchProp);
                                }
                                xmlFree(pitchProp);
                            }

                            float velocity = 0.0f;
                            {
                                char* velocityProp = (char*)xmlGetProp(nodeMessage, BAD_CAST XMLATTRIB_VELOCITY);
                                if (velocityProp) {
                                    velocity = atof(velocityProp);
                                }
                                xmlFree(velocityProp);
                            }

                            Block_addMidiMessage(self->blocks[self->nBlocks], type, time, pitch, velocity);
                        }
                    }
                    self->nBlocks++;
                }
            }
        }
    }
    if (self->nBlocks == 0) {
        puts("The loaded file does not contain any blocks, creating one");
        self->blocks[0] = Block_new(BLOCK_NAME_DEFAULT, COLOR_BLOCK_DEFAULT);
        self->nBlocks = 1;
    }

    /* Read tracks */
    for (xmlNode* node = nodeRoot->children; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_TRACKS, (char*)node->name)) {
            self->nTracks = 0;
            for (xmlNode* nodeTrack = node->children; nodeTrack; nodeTrack = nodeTrack ->next) {
                if (nodeTrack->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_TRACK, (char*)nodeTrack->name)) {
                    {
                        const char* programName = Synth_getDefaultProgramName(synth);
                        char* programProp = (char*)xmlGetProp(nodeTrack, BAD_CAST XMLATTRIB_PROGRAM);
                        if (programProp) {
                            programName = programProp;
                        }

                        float velocity = DEFAULT_VELOCITY;
                        char* velocityProp = (char*)xmlGetProp(nodeTrack, BAD_CAST XMLATTRIB_VELOCITY);
                        if (velocityProp) {
                            velocity = atof(velocityProp);
                        }

                        bool ignoreNoteOff = IGNORE_NOTE_OFF_DEFAULT;
                        char* ignoreNoteOffProp = (char*)xmlGetProp(nodeTrack, BAD_CAST XMLATTRIB_IGNORENOTEOFF);
                        if (ignoreNoteOffProp) {
                            ignoreNoteOff = atoi(ignoreNoteOffProp);
                        }

                        self->tracks[self->nTracks] = Track_new(programName, velocity, ignoreNoteOff);

                        xmlFree(programProp);
                        xmlFree(velocityProp);
                        xmlFree(ignoreNoteOffProp);
                    }

                    int iBlock = 0;
                    for (xmlNode* nodeBlock = nodeTrack->children; nodeBlock; nodeBlock = nodeBlock->next) {
                        if (nodeBlock->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCK, (char*)nodeBlock->name)) {
                            if (iBlock >= SCORE_LENGTH_MAX) die("Too many blocks (max is %d)", SCORE_LENGTH_MAX);
                            if (iBlock >= self->scoreLength) self->scoreLength = iBlock + 1;

                            char* nameProp = (char*)xmlGetProp(nodeBlock, BAD_CAST XMLATTRIB_NAME);
                            if (nameProp) {
                                printf("node block name='%s'\n", nameProp);
                                Block** block = NULL;
                                for (int i = 0; i < self->nBlocks; i++) {
                                    if (!self->blocks[i]->name) continue;
                                    if (!strcmp(self->blocks[i]->name, nameProp)) {
                                        block = &self->blocks[i];
                                        break;
                                    }
                                }

                                if (!block) {
                                    die("Did not find blockdef '%s'", nameProp);
                                }

                                float blockVelocity = DEFAULT_VELOCITY;
                                {
                                    char* blockVelocityProp = (char*)xmlGetProp(nodeBlock, BAD_CAST XMLATTRIB_VELOCITY);
                                    if (blockVelocityProp) {
                                        blockVelocity = atof(blockVelocityProp);
                                    }
                                    xmlFree(blockVelocityProp);
                                }

                                Track_setBlock(self->tracks[self->nTracks], iBlock, block);
                                Track_setBlockVelocity(self->tracks[self->nTracks], iBlock, blockVelocity);
                            }
                            else {
                                puts("empty block");
                                Track_setBlock(self->tracks[self->nTracks], iBlock, NULL);
                                Track_setBlockVelocity(self->tracks[self->nTracks], iBlock, DEFAULT_VELOCITY);
                            }
                            xmlFree(nameProp);
                            iBlock++;
                        }
                    }
                    self->nTracks++;
                }
            }
        }
    }
    if (self->nTracks == 0) {
        puts("The loaded file does not contain any tracks, creating one");
        self->tracks[0] = Track_new(Synth_getDefaultProgramName(synth), DEFAULT_VELOCITY, false);
        self->nTracks = 1;
        self->scoreLength = SCORE_LENGTH_DEFAULT;
    }

    xmlFreeDoc(doc);

    for (int iTrack = 0; iTrack < self->nTracks; iTrack++) {
        Synth_setProgramByName(synth, iTrack + 1, self->tracks[iTrack]->programName);
    }

    return self;
}


void Score_writeToFile(Score* self, const char* const filename) {
    xmlDocPtr doc = xmlNewDoc(BAD_CAST XML_VERSION);
    xmlNode* nodeScore = xmlNewNode(NULL, BAD_CAST XMLNODE_SCORE);
    xmlDocSetRootElement(doc, nodeScore);

    {
        char buffer[XML_BUFFER_SIZE];
        snprintf(buffer, XML_BUFFER_SIZE, "%d", self->tempo);
        xmlNewProp(nodeScore, BAD_CAST XMLATTRIB_TEMPO, BAD_CAST buffer);
    }

    xmlNewProp(nodeScore, BAD_CAST XMLATTRIB_KEYSIGNATURE, BAD_CAST KEY_SIGNATURE_NAMES[self->keySignature]);

    /* Creat xml nodes for block definitions */
    xmlNode* nodeBlockDefs = xmlNewChild(nodeScore, NULL, BAD_CAST XMLNODE_BLOCKDEFS, NULL);
    for (int iBlockDef = 0; iBlockDef < self->nBlocks; iBlockDef++) {
        const char* name = self->blocks[iBlockDef]->name;
        if (name) {
            xmlNode* nodeBlockDef = xmlNewChild(nodeBlockDefs, NULL, BAD_CAST XMLNODE_BLOCKDEF, NULL);
            xmlNewProp(nodeBlockDef, BAD_CAST XMLATTRIB_NAME, BAD_CAST name);
            xmlNewProp(nodeBlockDef, BAD_CAST XMLATTRIB_COLOR, BAD_CAST self->blocks[iBlockDef]->hexColor);
            for (MidiMessage* message = self->blocks[iBlockDef]->midiMessageRoot; message; message = message->next) {
                if (message == self->blocks[iBlockDef]->midiMessageRoot) continue;
                xmlNode* nodeMessage = xmlNewChild(nodeBlockDef, NULL, BAD_CAST XMLNODE_MESSAGE, NULL);
                char buffer[XML_BUFFER_SIZE];
                snprintf(buffer, XML_BUFFER_SIZE, "%f", message->time);
                xmlNewProp(nodeMessage, BAD_CAST XMLATTRIB_TIME, BAD_CAST buffer);
                snprintf(buffer, XML_BUFFER_SIZE, "%d", message->type);
                xmlNewProp(nodeMessage, BAD_CAST XMLATTRIB_TYPE, BAD_CAST buffer);
                snprintf(buffer, XML_BUFFER_SIZE, "%d", message->pitch);
                xmlNewProp(nodeMessage, BAD_CAST XMLATTRIB_PITCH, BAD_CAST buffer);
                snprintf(buffer, XML_BUFFER_SIZE, "%f", message->velocity);
                xmlNewProp(nodeMessage, BAD_CAST XMLATTRIB_VELOCITY, BAD_CAST buffer);
            }
        }
    }

    /* Create xml nodes for tracks */
    xmlNode* nodeTracks = xmlNewChild(nodeScore, NULL, BAD_CAST XMLNODE_TRACKS, NULL);
    for (int iTrack = 0; iTrack < self->nTracks; iTrack++) {
        xmlNode* nodeTrack = NULL; /* Lazy initialization */

        int nNullBlocks = 0;
        for (int iBlock = 0; iBlock < self->scoreLength; iBlock++) {
            if (self->tracks[iTrack]->blocks[iBlock]) {
                char buffer[XML_BUFFER_SIZE];
                if (!nodeTrack) {
                    nodeTrack = xmlNewChild(nodeTracks, NULL, BAD_CAST XMLNODE_TRACK, NULL);
                    xmlNewProp(nodeTrack, BAD_CAST XMLATTRIB_PROGRAM, BAD_CAST self->tracks[iTrack]->programName);
                    snprintf(buffer, XML_BUFFER_SIZE, "%f", self->tracks[iTrack]->velocity);
                    xmlNewProp(nodeTrack, BAD_CAST XMLATTRIB_VELOCITY, BAD_CAST buffer);
                    snprintf(buffer, XML_BUFFER_SIZE, "%d", self->tracks[iTrack]->ignoreNoteOff);
                    xmlNewProp(nodeTrack, BAD_CAST XMLATTRIB_IGNORENOTEOFF, BAD_CAST buffer);
                }
                for (int iNullBlock = 0; iNullBlock < nNullBlocks; iNullBlock++) {
                    xmlNewChild(nodeTrack, NULL, BAD_CAST XMLNODE_BLOCK, NULL);
                }
                xmlNode* nodeBlock = xmlNewChild(nodeTrack, NULL, BAD_CAST XMLNODE_BLOCK, NULL);
                Block* block = *self->tracks[iTrack]->blocks[iBlock];
                xmlNewProp(nodeBlock, BAD_CAST XMLATTRIB_NAME, BAD_CAST block->name);
                snprintf(buffer, XML_BUFFER_SIZE, "%f", self->tracks[iTrack]->blockVelocities[iBlock]);
                xmlNewProp(nodeBlock, BAD_CAST XMLATTRIB_VELOCITY, BAD_CAST buffer);
                nNullBlocks = 0;
            }
            else {
                nNullBlocks++;
            }
        }
    }

    xmlSaveFormatFileEnc(filename, doc, XML_ENCODING, 1);
    xmlFreeDoc(doc);
    xmlCleanupParser();
    printf("Wrote score to '%s'\n", filename);
}


void Score_regenerateBlockListString(Score* self) {
    memset(self->blockListString, 0, MAX_BLOCKS * (MAX_BLOCK_NAME_LENGTH + 1) + 1);
    for (int iBlock = 0; iBlock < self->nBlocks; iBlock++) {
        if (iBlock > 0) {
            strcat(self->blockListString, "\n");
        }
        strcat(self->blockListString, self->blocks[iBlock]->name);
    }
}


const char* Score_getBlockListString(void) {  /* called from input callback (no instance reference) */
    Score* self = Application_getInstance()->scoreCurrent;
    Score_regenerateBlockListString(self);
    return self->blockListString;
}


void Score_setBlockByName(Score* self, const char* const name) {
    Application* application = Application_getInstance();

    for (int iBlock = 0; iBlock < self->nBlocks; iBlock++) {
        if (!strcmp(name, self->blocks[iBlock]->name)) {
            Application_switchBlock(application, &self->blocks[iBlock]);
            return;
        }
    }

    if (self->nBlocks == MAX_BLOCKS) {
        printf("Already at maximum number of blocks (%d)\n", MAX_BLOCKS);
        return;
    }

    printf("Creating new block '%s'\n", name);
    self->blocks[self->nBlocks] = Block_new(name, COLOR_BLOCK_DEFAULT);
    Application_switchBlock(application, &self->blocks[self->nBlocks]);
    self->nBlocks++;
}


const char* Score_getCurrentBlockName(void) {  /* called from input callback (no instance reference) */
    Block* blockCurrent = *Application_getInstance()->blockCurrent;
    return blockCurrent->name;
}


void Score_renameBlock(Score* self, const char* const name) {
    for (int iBlock = 0; iBlock < self->nBlocks; iBlock++) {
        if (!strcmp(name, self->blocks[iBlock]->name)) {
            if (self->blocks[iBlock] == *Application_getInstance()->blockCurrent) {
                printf("Block name '%s' unchanged\n", name);
            }
            else {
                printf("Block with name '%s' already exists\n", name);
            }
            return;
        }
    }

    Block* blockCurrent = *Application_getInstance()->blockCurrent;
    printf("Renaming block from '%s' to '%s'\n", blockCurrent->name, name);
    Block_setName(blockCurrent, name);
}


const char* Score_getCurrentBlockColor(void) {  /* called from input callback (no instance reference) */
    Block* blockCurrent = *Application_getInstance()->blockCurrent;
    return blockCurrent->hexColor;
}


void Score_setBlockColor(const char* const hexColor) {
    Block* blockCurrent = *Application_getInstance()->blockCurrent;
    Block_setColor(blockCurrent, hexColor);
}


void Score_increaseLength(Score* self) {
    if (self->scoreLength == SCORE_LENGTH_MAX) {
        printf("Already at max score length (%d)\n", SCORE_LENGTH_MAX);
        return;
    }
    self->scoreLength++;
    printf("Score length increased to %d blocks\n", self->scoreLength);
}


void Score_decreaseLength(Score* self) {
    if (self->scoreLength == 1) {
        printf("Already at minimum score length (%d)\n", 1);
        return;
    }
    self->scoreLength--;
    printf("Score length decreased to %d blocks\n", self->scoreLength);
}


void Score_addTrack(Score* self) {
    if (self->nTracks == N_TRACKS_MAX) {
        printf("Already at maximum number of tracks (%d)\n", N_TRACKS_MAX);
        return;
    }
    if (!self->tracks[self->nTracks]) {
        const char* const programName = Synth_getDefaultProgramName(Application_getInstance()->synth);
        self->tracks[self->nTracks] = Track_new(programName, DEFAULT_VELOCITY, false);
    }
    self->nTracks++;
    printf("Number of tracks increased to %d\n", self->nTracks);
}


void Score_removeTrack(Score* self) {
    if (self->nTracks == 1) {
        printf("Already at minimum number of tracks (%d)\n", 1);
        return;
    }
    self->nTracks--;
    printf("Number of tracks decreased to %d\n", self->nTracks);
}


const char* Score_getKeySignatureName(void) {
    return KEY_SIGNATURE_LIST_STRING;
}


void Score_setKeySignatureByName(Score* self, const char* const name) {
    for (int iKey = 0; iKey < KEY_SIGNATURE_COUNT; iKey++) {
        if (!strcmp(name, KEY_SIGNATURE_NAMES[iKey])) {
            self->keySignature = iKey;
            return;
        }
    }
    printf("Error: Invalid key signature '%s'\n", name);
}
