CC=gcc
CFLAGS=-std=c99 -O3 -pedantic-errors -Wall -Wextra -Werror=vla -Werror=implicit-fallthrough -DGLEW_NO_GLU
CFILES=canvas.c config.h gscore.c gscore.h input.c main.c player.c renderer.c synth.c util.c

LIBS=-lGL -lGLEW -lglfw -lfluidsynth

gscore: $(CFILES)
	$(CC) $(CFLAGS) -o gscore gscore.c $(LIBS)

clean:
	rm -f gscore
