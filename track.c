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

static Track* Track_new(const char* const programName, float velocity, bool ignoreNoteOff) {
    Track* self = ecalloc(1, sizeof(*self));

    Track_setProgram(self, programName);

    self->velocity = velocity;
    self->ignoreNoteOff = ignoreNoteOff;

    for (int iBlock = 0; iBlock < SCORE_LENGTH_MAX; iBlock++) {
        self->blocks[iBlock] = NULL;
        self->blockVelocities[iBlock] = 0.75f;
    }

    return self;
}


static Track* Track_free(Track* self) {
    free(self);
    return NULL;
}


static void Track_setProgram(Track* self, const char* const programName) {
    strncpy(self->programName, programName, SYNTH_PROGRAM_NAME_LENGTH_MAX);
    self->programName[SYNTH_PROGRAM_NAME_LENGTH_MAX - 1] = '\0';
}


static void Track_setVelocity(Track* self, float velocity) {
    self->velocity = velocity;
}


static void Track_setBlock(Track* self, int iBlock, Block** block) {
    self->blocks[iBlock] = block;
}


static void Track_setBlockVelocity(Track* self, int iBlock, float blockVelocity) {
    self->blockVelocities[iBlock] = blockVelocity;
}


static void Track_adjustBlockVelocity(Track* self, int iBlock, float amount) {
    if (!self->blocks[iBlock]) return;
    self->blockVelocities[iBlock] += amount;
    if (self->blockVelocities[iBlock] > 1.0f) self->blockVelocities[iBlock] = 1.0f;
    if (self->blockVelocities[iBlock] < 0.0f) self->blockVelocities[iBlock] = 0.0f;
    printf("Block velocity: %f\n", self->blockVelocities[iBlock]);
}


static void Track_toggleIgnoreNoteOff(Track* self) {
    self->ignoreNoteOff = !self->ignoreNoteOff;
    printf("%s note off events for track\n", self->ignoreNoteOff ? "Disabled" : "Enabled");
}
