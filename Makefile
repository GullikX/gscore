CC=cc
#CFLAGS=-O3 -march=native
CFLAGS=-Og -g -march=native

CFILES=application.c blockplayer.c config.h editview.c filereader.c filewriter.c gscore.c gscore.h input.c main.c objectview.c renderer.c scoreplayer.c synth.c util.c xevents.c
INCLUDE=$$(xml2-config --cflags)
LIBS=-lGL -lGLEW -lglfw -lfluidsynth -lX11 $$(xml2-config --libs)

WARNINGS=-Wall -Wextra
ERRORS=-pedantic-errors -Werror=vla -Werror=implicit-fallthrough -Werror=strict-prototypes
DEFINES=-DGLEW_NO_GLU -DGLFW_EXPOSE_NATIVE_X11 -D_POSIX_SOURCE
OPTS=-std=c99 $(WARNINGS) $(ERRORS) $(DEFINES)

gscore: $(CFILES)
	$(CC) $(CFLAGS) $(INCLUDE) $(OPTS) -o gscore gscore.c $(LIBS)

clean:
	rm -f gscore
