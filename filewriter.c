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

void FileWriter_write(const Score* const score, const char* const filename) {
    xmlDocPtr doc = xmlNewDoc(BAD_CAST XML_VERSION);
    xmlNode* nodeScore = xmlNewNode(NULL, BAD_CAST XMLNODE_SCORE);
    xmlDocSetRootElement(doc, nodeScore);

    char buffer[XML_BUFFER_SIZE];
    snprintf(buffer, XML_BUFFER_SIZE, "%d", score->tempo);
    xmlNewProp(nodeScore, BAD_CAST XMLATTRIB_TEMPO, BAD_CAST buffer);

    FileWriter_writeBlockDefs(score, nodeScore);
    FileWriter_writeTracks(score, nodeScore);

    xmlSaveFormatFileEnc(filename, doc, XML_ENCODING, 1);
    xmlFreeDoc(doc);
    xmlCleanupParser();
    printf("Wrote score to '%s'\n", filename);
}


void FileWriter_writeBlockDefs(const Score* const score, xmlNode* nodeScore) {
    xmlNode* nodeBlockDefs = xmlNewChild(nodeScore, NULL, BAD_CAST XMLNODE_BLOCKDEFS, NULL);
    for (int iBlockDef = 0; iBlockDef < MAX_BLOCKS; iBlockDef++) {
        const char* name = score->blocks[iBlockDef].name;
        if (name) {
            xmlNode* nodeBlockDef = xmlNewChild(nodeBlockDefs, NULL, BAD_CAST XMLNODE_BLOCKDEF, NULL);
            xmlNewProp(nodeBlockDef, BAD_CAST XMLATTRIB_NAME, BAD_CAST name);
            for (MidiMessage* message = score->blocks[iBlockDef].midiMessageRoot; message; message = message->next) {
                if (message == score->blocks[iBlockDef].midiMessageRoot) continue;
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
}


void FileWriter_writeTracks(const Score* const score, xmlNode* nodeScore) {
    xmlNode* nodeTracks = xmlNewChild(nodeScore, NULL, BAD_CAST XMLNODE_TRACKS, NULL);
    for (int iTrack = 0; iTrack < N_TRACKS; iTrack++) {
        xmlNode* nodeTrack = NULL;

        int nNullBlocks = 0;
        for (int iBlock = 0; iBlock < SCORE_LENGTH; iBlock++) {
            if (score->tracks[iTrack].blocks[iBlock]) {
                if (!nodeTrack) {
                    nodeTrack = xmlNewChild(nodeTracks, NULL, BAD_CAST XMLNODE_TRACK, NULL);
                    char buffer[XML_BUFFER_SIZE];
                    snprintf(buffer, XML_BUFFER_SIZE, "%d", score->tracks[iTrack].program);
                    xmlNewProp(nodeTrack, BAD_CAST XMLATTRIB_PROGRAM, BAD_CAST buffer);
                    snprintf(buffer, XML_BUFFER_SIZE, "%f", score->tracks[iTrack].velocity);
                    xmlNewProp(nodeTrack, BAD_CAST XMLATTRIB_VELOCITY, BAD_CAST buffer);
                }
                for (int iNullBlock = 0; iNullBlock < nNullBlocks; iNullBlock++) {
                    xmlNewChild(nodeTrack, NULL, BAD_CAST XMLNODE_BLOCK, NULL);
                }
                xmlNode* nodeBlock = xmlNewChild(nodeTrack, NULL, BAD_CAST XMLNODE_BLOCK, NULL);
                xmlNewProp(nodeBlock, BAD_CAST XMLATTRIB_NAME, BAD_CAST score->tracks[iTrack].blocks[iBlock]->name);
                nNullBlocks = 0;
            }
            else {
                nNullBlocks++;
            }
        }
    }
}
