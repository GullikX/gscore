/* Copyright (C) 2020 Martin Gulliksson <martin@gullik.cc>
 *
 * This file is part of GScore.
 *
 * GScore is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, version 3.
 *
 * GScore is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>
 *
 */

Score* FileReader_read(const char* const filename) {
    if (fileExists(filename)) {
        printf("Loading file '%s'...\n", filename);
        return FileReader_createScoreFromFile(filename);
    }
    else {
        printf("File '%s' does not exist, creating new empty score...\n", filename);
        return FileReader_createNewEmptyScore();
    }
}


Score* FileReader_createScoreFromFile(const char* const filename) {
    Score* score = ecalloc(1, sizeof(*score));
    xmlDocPtr doc = xmlReadFile(filename, NULL, 0);
    xmlNode* nodeRoot = xmlDocGetRootElement(doc);
    if (!nodeRoot) {
        die("Failed to parse input file '%s'", filename);
    }
    if (strcmp(XMLNODE_SCORE, (char*)nodeRoot->name)) {
        die("Unexpected node name '%s', expected '%s'", (char*)nodeRoot->name, XMLNODE_SCORE);
    }

    score->tempo = atoi((char*)xmlGetProp(nodeRoot, BAD_CAST XMLATTRIB_TEMPO));
    if (!score->tempo) die("Invalid tempo value");

    for (xmlNode* node = nodeRoot->children; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCKDEFS, (char*)node->name)) {
            FileReader_createBlockDefs(score, node);
        }
    }

    for (xmlNode* node = nodeRoot->children; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_TRACKS, (char*)node->name)) {
            FileReader_createTracks(score, node);
        }
    }

    xmlFreeDoc(doc);
    return score;
}


void FileReader_createBlockDefs(Score* score, xmlNode* nodeBlockDefs) {
    int iBlock = 0;
    for (xmlNode* nodeBlockDef = nodeBlockDefs->children; nodeBlockDef; nodeBlockDef = nodeBlockDef->next) {
        if (nodeBlockDef->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCKDEF, (char*)nodeBlockDef->name)) {
            if (iBlock >= MAX_BLOCKS) die("To many blocks (max is %d)", MAX_BLOCKS);
            score->blocks[iBlock].name = (char*)xmlGetProp(nodeBlockDef, BAD_CAST XMLATTRIB_NAME);
            score->blocks[iBlock].color = COLOR_BLOCK_DEFAULT;

            score->blocks[iBlock].midiMessageRoot = ecalloc(1, sizeof(MidiMessage));
            score->blocks[iBlock].midiMessageRoot->type = FLUID_SEQ_NOTE;
            score->blocks[iBlock].midiMessageRoot->time = -1.0f;
            score->blocks[iBlock].midiMessageRoot->pitch = -1;
            score->blocks[iBlock].midiMessageRoot->velocity = -1;
            score->blocks[iBlock].midiMessageRoot->next = NULL;
            score->blocks[iBlock].midiMessageRoot->prev = NULL;

            MidiMessage* message = score->blocks[iBlock].midiMessageRoot;
            for (xmlNode* nodeMessage = nodeBlockDef->children; nodeMessage; nodeMessage = nodeMessage->next) {
                if (nodeMessage->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_MESSAGE, (char*)nodeMessage->name)) {
                    /* TODO: error checking */
                    message->next = ecalloc(1, sizeof(MidiMessage));
                    message->next->type = atoi((char*)xmlGetProp(nodeMessage, BAD_CAST XMLATTRIB_TYPE));
                    message->next->time = atof((char*)xmlGetProp(nodeMessage, BAD_CAST XMLATTRIB_TIME));
                    message->next->pitch = atoi((char*)xmlGetProp(nodeMessage, BAD_CAST XMLATTRIB_PITCH));
                    message->next->velocity = atoi((char*)xmlGetProp(nodeMessage, BAD_CAST XMLATTRIB_VELOCITY));
                    message->next->next = NULL;
                    message->next->prev = message;
                    message = message->next;
                }
            }
            iBlock++;
        }
    }
}


void FileReader_createTracks(Score* score, xmlNode* nodeTracks) {
    int iTrack = 0;
    for (xmlNode* nodeTrack = nodeTracks->children; nodeTrack; nodeTrack = nodeTrack ->next) {
        if (nodeTrack->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_TRACK, (char*)nodeTrack->name)) {
            int iBlock = 0;
            printf("program %s\n", (char*)xmlGetProp(nodeTrack, BAD_CAST XMLATTRIB_PROGRAM));
            score->tracks[iTrack].program = atoi((char*)xmlGetProp(nodeTrack, BAD_CAST XMLATTRIB_PROGRAM));
            for (xmlNode* nodeBlock = nodeTrack->children; nodeBlock; nodeBlock = nodeBlock->next) {
                if (nodeBlock->type == XML_ELEMENT_NODE && !strcmp(XMLNODE_BLOCK, (char*)nodeBlock->name)) {
                    if (iBlock >= MAX_BLOCKS) die("To many blocks (max is %d)", MAX_BLOCKS);
                    const char* name = (char*)xmlGetProp(nodeBlock, BAD_CAST XMLATTRIB_NAME);
                    printf("node block name='%s'\n", name);
                    if (name) {
                        Block* block = NULL;
                        for (int i = 0; i < MAX_BLOCKS; i++) {
                            if (!score->blocks[i].name) continue;
                            if (!strcmp(score->blocks[i].name, name)) {
                                block = &score->blocks[i];
                                break;
                            }
                        }
                        if (!block) die("Did not find blockdef '%s'", name);
                        score->tracks[iTrack].blocks[iBlock] = block;
                    }
                    else {
                        score->tracks[iTrack].blocks[iBlock] = NULL;
                    }
                    iBlock++;
                }
            }
            iTrack++;
        }
    }
}


Score* FileReader_createNewEmptyScore() {
    Score* score = ecalloc(1, sizeof(*score));
    score->tempo = TEMPO_BPM;
    score->blocks[0].name = BLOCK_NAME_DEFAULT;
    score->blocks[0].color = COLOR_BLOCK_DEFAULT;
    score->blocks[0].midiMessageRoot = ecalloc(1, sizeof(MidiMessage));
    score->blocks[0].midiMessageRoot->type = FLUID_SEQ_NOTE;
    score->blocks[0].midiMessageRoot->time = -1.0f;
    score->blocks[0].midiMessageRoot->pitch = -1;
    score->blocks[0].midiMessageRoot->velocity = -1;
    score->blocks[0].midiMessageRoot->next = NULL;
    score->blocks[0].midiMessageRoot->prev = NULL;
    return score;
}
