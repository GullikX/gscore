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

static Block* Block_new(const char* const name, const char* const hexColor) {
    Block* self = ecalloc(1, sizeof(*self));

    Block_setName(self, name);
    if (hexColor) {
        Block_setColor(self, hexColor);
    }
    else {
        self->color = COLOR_BLOCK_DEFAULT;

        if (strcmp(name, BLOCK_NAME_DEFAULT)) {
            /* Add some color variation */
            int hash = djb2(self->name);
            int rOffset = (hash >> 16) & 0xff;
            int gOffset = (hash >> 8) & 0xff;
            int bOffset = hash & 0xff;
            self->color.x += BLOCK_NEW_COLOR_VARIATION * (float)rOffset / 255.0f;
            self->color.y += BLOCK_NEW_COLOR_VARIATION * (float)gOffset / 255.0f;
            self->color.z += BLOCK_NEW_COLOR_VARIATION * (float)bOffset / 255.0f;

            if (self->color.x > 1.0f) self->color.x = 1.0f;
            if (self->color.y > 1.0f) self->color.y = 1.0f;
            if (self->color.z > 1.0f) self->color.z = 1.0f;
            if (self->color.x < 0.0f) self->color.x = 0.0f;
            if (self->color.y < 0.0f) self->color.y = 0.0f;
            if (self->color.z < 0.0f) self->color.z = 0.0f;
        }
    }

    self->midiMessageRoot = ecalloc(1, sizeof(MidiMessage));
    self->midiMessageRoot->type = FLUID_SEQ_NOTE;
    self->midiMessageRoot->time = -1.0f;
    self->midiMessageRoot->pitch = -1;
    self->midiMessageRoot->velocity = -1.0f;
    self->midiMessageRoot->next = NULL;
    self->midiMessageRoot->prev = NULL;

    return self;
}


static Block* Block_free(Block* self) {
    MidiMessage* midiMessage = self->midiMessageRoot;
    self->midiMessageRoot = NULL;

    while(midiMessage) {
        MidiMessage* midiMessageTemp = midiMessage;
        midiMessage = midiMessage->next;
        free(midiMessageTemp);
    }

    free(self);
    return NULL;
}


static void Block_setName(Block* self, const char* const name) {
    strncpy(self->name, name, MAX_BLOCK_NAME_LENGTH);
    self->name[MAX_BLOCK_NAME_LENGTH - 1] = '\0';
}


static void Block_setColor(Block* self, const char* const hexColor) {
    bool success = hexColorToRgb(hexColor, &self->color);
    if (success) {
        printf("Set block color of '%s' to '%s'\n", self->name, hexColor);
    }
    else {
        warn("Failed to set block color '%s'", hexColor);
    }

    if (self->color.x > 1.0f) self->color.x = 1.0f;
    if (self->color.y > 1.0f) self->color.y = 1.0f;
    if (self->color.z > 1.0f) self->color.z = 1.0f;
    if (self->color.x < 0.0f) self->color.x = 0.0f;
    if (self->color.y < 0.0f) self->color.y = 0.0f;
    if (self->color.z < 0.0f) self->color.z = 0.0f;
}


static MidiMessage* Block_addMidiMessage(Block* self, int type, float time, int pitch, float velocity) {
    MidiMessage* midiMessage = ecalloc(1, sizeof(MidiMessage));
    midiMessage->type = type;
    midiMessage->time = time;
    midiMessage->pitch = pitch;
    midiMessage->velocity = velocity;
    midiMessage->next = NULL;
    midiMessage->prev = NULL;

    MidiMessage* midiMessageOther = self->midiMessageRoot;
    while (midiMessageOther->next && !Block_compareMidiMessages(midiMessageOther->next, midiMessage)) {
        midiMessageOther = midiMessageOther->next;
    }
    midiMessage->next = midiMessageOther->next;
    midiMessage->prev = midiMessageOther;
    if (midiMessageOther->next) midiMessageOther->next->prev = midiMessage;
    midiMessageOther->next = midiMessage;

    return midiMessage;
}


static void Block_removeMidiMessage(MidiMessage* midiMessage) {
    if (!midiMessage) return;
    if (midiMessage->prev) midiMessage->prev->next = midiMessage->next;
    if (midiMessage->next) midiMessage->next->prev = midiMessage->prev;
    free(midiMessage);
}


static bool Block_compareMidiMessages(MidiMessage* midiMessage, MidiMessage* midiMessageOther) {
    if (midiMessage->time > midiMessageOther->time) return true;
    else if (midiMessage->time < midiMessageOther->time) return false;
    else if (midiMessage->type == FLUID_SEQ_NOTEON && midiMessageOther->type == FLUID_SEQ_NOTEOFF) return true;
    else if (midiMessage->type == FLUID_SEQ_NOTEOFF && midiMessageOther->type == FLUID_SEQ_NOTEON) return false;
    else if (midiMessage->pitch > midiMessageOther->pitch) return true;
    else if (midiMessage->pitch < midiMessageOther->pitch) return false;
    else warn("Overlapping midi messages at time=%f, type=%d, pitch=%d", midiMessage->time, midiMessage->type, midiMessage->pitch);
    return true;
}
