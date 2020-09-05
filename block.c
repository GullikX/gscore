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

Block* Block_new(const char* const name, const Vector4* const color) {
    Block* self = ecalloc(1, sizeof(*self));

    int nameLength = strlen(name) + 1;
    self->name = ecalloc(nameLength, sizeof(char));
    strcpy(self->name, name);

    memcpy(&self->color, color, sizeof(Vector4));

    self->midiMessageRoot = ecalloc(1, sizeof(MidiMessage));
    self->midiMessageRoot->type = FLUID_SEQ_NOTE;
    self->midiMessageRoot->time = -1.0f;
    self->midiMessageRoot->pitch = -1;
    self->midiMessageRoot->velocity = -1.0f;
    self->midiMessageRoot->next = NULL;
    self->midiMessageRoot->prev = NULL;

    return self;
}


Block* Block_free(Block* self) {
    free(self->name);
    free(self);
    return NULL;
}


MidiMessage* Block_addMidiMessage(Block* self, int type, float time, int pitch, float velocity) {
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


void Block_removeMidiMessage(MidiMessage* midiMessage) {
    if (!midiMessage) return;
    if (midiMessage->prev) midiMessage->prev->next = midiMessage->next;
    if (midiMessage->next) midiMessage->next->prev = midiMessage->prev;
    free(midiMessage);
}


bool Block_compareMidiMessages(MidiMessage* midiMessage, MidiMessage* midiMessageOther) {
    if (midiMessage->time > midiMessageOther->time) return true;
    else if (midiMessage->time < midiMessageOther->time) return false;
    else if (midiMessage->type == FLUID_SEQ_NOTEON && midiMessageOther->type == FLUID_SEQ_NOTEOFF) return true;
    else if (midiMessage->type == FLUID_SEQ_NOTEOFF && midiMessageOther->type == FLUID_SEQ_NOTEON) return false;
    else if (midiMessage->pitch > midiMessageOther->pitch) return true;
    else if (midiMessage->pitch < midiMessageOther->pitch) return false;
    else printf("Overlapping midi messages at time=%f, type=%d, pitch=%d\n", midiMessage->time, midiMessage->type, midiMessage->pitch);
    return true;
}
