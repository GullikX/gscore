CC=gcc
CFLAGS=-std=c99 -O3 -pedantic-errors -Wall -Wextra -Werror=vla -Werror=implicit-fallthrough -Werror=strict-prototypes -DGLEW_NO_GLU -DGLFW_EXPOSE_NATIVE_X11
CFILES=canvas.c config.h gscore.c gscore.h input.c main.c player.c renderer.c synth.c util.c xevents.c

LIBS=-lGL -lGLEW -lglfw -lfluidsynth -lX11

gscore: $(CFILES)
	$(CC) $(CFLAGS) -o gscore gscore.c $(LIBS)

clean:
	rm -f gscore
