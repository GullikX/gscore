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

Score* Score_new(void) {
    Score* self = ecalloc(1, sizeof(*self));
    self->tempo = TEMPO_BPM;

    self->blocks[0] = Block_new(BLOCK_NAME_DEFAULT, COLOR_BLOCK_DEFAULT);
    self->nBlocks = 1;

    self->tracks[0] = Track_new(SYNTH_PROGRAM_DEFAULT, DEFAULT_VELOCITY);
    self->nTracks = 1;

    self->scoreLength = SCORE_LENGTH_DEFAULT;

    return self;
}


Score* Score_free(Score* self) {
    free(self);
    return NULL;
}


Score* Score_readFromFile(const char* const filename) {
    Score* self = ecalloc(1, sizeof(*self));
    xmlDocPtr doc = xmlReadFile(filename, NULL, 0);
    xmlNode* nodeRoot = xmlDocGetRootElement(doc);
    if (!nodeRoot) {
        die("Failed to parse input file '%s'", filename);
    }
    if (strcmp(XMLNODE_SCORE, (char*)nodeRoot->name)) {
        die("Unexpected node name '%s', expected '%s'", (char*)nodeRoot->name, XMLNODE_SCORE);
    }

    self->tempo = atoi((char*)xmlGetProp(nodeRoot, BAD_CAST XMLATTRIB_TEMPO));
    if (!self->tempo) die("Invalid tempo value");

    /* Read block definitions */
    for (xmlNode* node = nodeRoot->children; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCKDEFS, (char*)node->name)) {
            self->nBlocks = 0;
            for (xmlNode* nodeBlockDef = node->children; nodeBlockDef; nodeBlockDef = nodeBlockDef->next) {
                if (nodeBlockDef->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCKDEF, (char*)nodeBlockDef->name)) {
                    if (self->nBlocks >= MAX_BLOCKS) die("To many blocks (max is %d)", MAX_BLOCKS);
                    const char* const name = (char*)xmlGetProp(nodeBlockDef, BAD_CAST XMLATTRIB_NAME);
                    const char* const hexColor = (char*)xmlGetProp(nodeBlockDef, BAD_CAST XMLATTRIB_COLOR);
                    self->blocks[self->nBlocks] = Block_new(name, hexColor);

                    for (xmlNode* nodeMessage = nodeBlockDef->children; nodeMessage; nodeMessage = nodeMessage->next) {
                        if (nodeMessage->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_MESSAGE, (char*)nodeMessage->name)) {
                            /* TODO: error checking */
                            int type = atoi((char*)xmlGetProp(nodeMessage, BAD_CAST XMLATTRIB_TYPE));
                            float time = atof((char*)xmlGetProp(nodeMessage, BAD_CAST XMLATTRIB_TIME));
                            int pitch = atoi((char*)xmlGetProp(nodeMessage, BAD_CAST XMLATTRIB_PITCH));
                            float velocity = atof((char*)xmlGetProp(nodeMessage, BAD_CAST XMLATTRIB_VELOCITY));
                            Block_addMidiMessage(self->blocks[self->nBlocks], type, time, pitch, velocity);
                        }
                    }
                    self->nBlocks++;
                }
            }
        }
    }

    /* Read tracks */
    for (xmlNode* node = nodeRoot->children; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_TRACKS, (char*)node->name)) {
            self->nTracks = 0;
            for (xmlNode* nodeTrack = node->children; nodeTrack; nodeTrack = nodeTrack ->next) {
                if (nodeTrack->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_TRACK, (char*)nodeTrack->name)) {
                    int program = atoi((char*)xmlGetProp(nodeTrack, BAD_CAST XMLATTRIB_PROGRAM));
                    float velocity = atof((char*)xmlGetProp(nodeTrack, BAD_CAST XMLATTRIB_VELOCITY));
                    self->tracks[self->nTracks] = Track_new(program, velocity);

                    int iBlock = 0;
                    for (xmlNode* nodeBlock = nodeTrack->children; nodeBlock; nodeBlock = nodeBlock->next) {
                        if (nodeBlock->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCK, (char*)nodeBlock->name)) {
                            if (iBlock >= SCORE_LENGTH_MAX) die("Too many blocks (max is %d)", SCORE_LENGTH_MAX);
                            if (iBlock >= self->scoreLength) self->scoreLength = iBlock + 1;

                            const char* name = (char*)xmlGetProp(nodeBlock, BAD_CAST XMLATTRIB_NAME);
                            printf("node block name='%s'\n", name);
                            if (name) {
                                Block** block = NULL;
                                for (int i = 0; i < self->nBlocks; i++) {
                                    if (!self->blocks[i]->name) continue;
                                    if (!strcmp(self->blocks[i]->name, name)) {
                                        block = &self->blocks[i];
                                        break;
                                    }
                                }
                                if (!*block) die("Did not find blockdef '%s'", name);
                                float blockVelocity = atof((char*)xmlGetProp(nodeBlock, BAD_CAST XMLATTRIB_VELOCITY));
                                Track_setBlock(self->tracks[self->nTracks], iBlock, block);
                                Track_setBlockVelocity(self->tracks[self->nTracks], iBlock, blockVelocity);
                            }
                            else {
                                Track_setBlock(self->tracks[self->nTracks], iBlock, NULL);
                                Track_setBlockVelocity(self->tracks[self->nTracks], iBlock, DEFAULT_VELOCITY);
                            }
                            iBlock++;
                        }
                    }
                    self->nTracks++;
                }
            }
        }
    }

    xmlFreeDoc(doc);
    return self;
}


void Score_writeToFile(Score* self, const char* const filename) {
    xmlDocPtr doc = xmlNewDoc(BAD_CAST XML_VERSION);
    xmlNode* nodeScore = xmlNewNode(NULL, BAD_CAST XMLNODE_SCORE);
    xmlDocSetRootElement(doc, nodeScore);

    char buffer[XML_BUFFER_SIZE];
    snprintf(buffer, XML_BUFFER_SIZE, "%d", self->tempo);
    xmlNewProp(nodeScore, BAD_CAST XMLATTRIB_TEMPO, BAD_CAST buffer);

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
                    snprintf(buffer, XML_BUFFER_SIZE, "%d", self->tracks[iTrack]->program);
                    xmlNewProp(nodeTrack, BAD_CAST XMLATTRIB_PROGRAM, BAD_CAST buffer);
                    snprintf(buffer, XML_BUFFER_SIZE, "%f", self->tracks[iTrack]->velocity);
                    xmlNewProp(nodeTrack, BAD_CAST XMLATTRIB_VELOCITY, BAD_CAST buffer);
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
    for (int iBlock = 0; iBlock < MAX_BLOCKS; iBlock++) {
        if (iBlock > 0) {
            strcat(self->blockListString, "\n");
        }
        strcat(self->blockListString, self->blocks[iBlock]->name);
    }
}


char* Score_getBlockListString(void) {  /* called from input callback (no instance reference) */
    Score* self = Application_getInstance()->scoreCurrent;
    Score_regenerateBlockListString(self);
    return self->blockListString;
}


void Score_setBlockByName(Score* self, const char* const name) {
    for (int iBlock = 0; iBlock < MAX_BLOCKS; iBlock++) {
        if (!strcmp(name, self->blocks[iBlock]->name)) {
            printf("Switching to block '%s'\n", name);
            Application_getInstance()->blockCurrent = &self->blocks[iBlock];
            return;
        }
    }

    printf("Creating new block '%s'\n", name);
    /* TODO */
}


char* Score_getCurrentBlockName(void) {  /* called from input callback (no instance reference) */
    Block* blockCurrent = *Application_getInstance()->blockCurrent;
    return blockCurrent->name;
}


void Score_renameBlock(Score* self, const char* const name) {
    for (int iBlock = 0; iBlock < MAX_BLOCKS; iBlock++) {
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


char* Score_getCurrentBlockColor(void) {  /* called from input callback (no instance reference) */
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
}

void Score_decreaseLength(Score* self) {
    if (self->scoreLength == 1) {
        printf("Already at minimum score length (%d)\n", 1);
        return;
    }
    self->scoreLength--;
}
