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
    xmlNode* nodeRoot = xmlNewNode(NULL, BAD_CAST XMLNODE_GSCORE);
    xmlDocSetRootElement(doc, nodeRoot);

    FileWrite_writeBlockDefs(score, nodeRoot);
    FileWrite_writeTracks(score, nodeRoot);

    xmlSaveFormatFileEnc("-", doc, XML_ENCODING, 1);
    xmlFreeDoc(doc);
    xmlCleanupParser();
}


void FileWrite_writeBlockDefs(const Score* const score, xmlNode* nodeRoot) {
    xmlNode* nodeBlockDefs = xmlNewChild(nodeRoot, NULL, BAD_CAST XMLNODE_BLOCKDEFS, NULL);
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
                snprintf(buffer, XML_BUFFER_SIZE, "%d", message->velocity);
                xmlNewProp(nodeMessage, BAD_CAST XMLATTRIB_VELOCITY, BAD_CAST buffer);
            }
        }
    }
}

void FileWrite_writeTracks(const Score* const score, xmlNode* nodeRoot) {

}
