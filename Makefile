CC=cc
CFLAGS=-O3 -march=native

CFILES=application.c canvas.c config.h gscore.c gscore.h input.c main.c player.c renderer.c synth.c util.c xevents.c
LIBS=-lGL -lGLEW -lglfw -lfluidsynth -lX11

WARNINGS=-Wall -Wextra
ERRORS=-pedantic-errors -Werror=vla -Werror=implicit-fallthrough -Werror=strict-prototypes
DEFINES=-DGLEW_NO_GLU -DGLFW_EXPOSE_NATIVE_X11 -D_POSIX_SOURCE
OPTS=-std=c99 $(WARNINGS) $(ERRORS) $(DEFINES)

gscore: $(CFILES)
	$(CC) $(CFLAGS) $(OPTS) -o gscore gscore.c $(LIBS)

clean:
	rm -f gscore
