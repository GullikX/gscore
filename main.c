/* Copyright (C) 2020-2021 Martin Gulliksson <martin@gullik.cc>
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

int main(int argc, char* argv[]) {
    puts("This is gscore "VERSION);
    if (argc != 2) {
        printf("usage: %s filename\n", argv[0]);
        return 1;
    }
    Application* application = Application_new(argv[1]);
    Application_run(application);
    application = Application_free(application);
}
