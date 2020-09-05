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

Track* Track_new(int program, float velocity) {
    Track* self = ecalloc(1, sizeof(*self));

    self->program = program;
    self->velocity = velocity;

    for (int iBlock = 0; iBlock < SCORE_LENGTH; iBlock++) {
        self->blocks[iBlock] = NULL;
        self->blockVelocities[iBlock] = 0.75f;
    }

    return self;
}


Track* Track_free(Track* self) {
    free(self);
    return NULL;
}


void Track_setProgram(Track* self, int program) {
    self->program = program;
}


void Track_setVelocity(Track* self, float velocity) {
    self->velocity = velocity;
}


void Track_setBlock(Track* self, int iBlock, Block** block) {
    self->blocks[iBlock] = block;
}


void Track_setBlockVelocity(Track* self, int iBlock, float blockVelocity) {
    self->blockVelocities[iBlock] = blockVelocity;
}
