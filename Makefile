CC?=cc
#CFLAGS?=-O3 -march=native
CFLAGS?=-Og -g -march=native

CFILES=application.c block.c editview.c gscore.c input.c main.c objectview.c renderer.c score.c synth.c track.c util.c xevents.c
HFILES=config.h gscore.h
INCLUDE=$$(xml2-config --cflags)
LIBS=-lGL -lGLEW -lglfw -lfluidsynth -lX11 $$(xml2-config --libs)

WARNINGS=-Wall -Wextra
ERRORS=-pedantic -Werror=vla -Werror=implicit-fallthrough -Werror=strict-prototypes
DEFINES=-DGLEW_NO_GLU -DGLFW_EXPOSE_NATIVE_X11 -D_POSIX_SOURCE
OPTS=-std=c99 $(WARNINGS) $(ERRORS) $(DEFINES)

gscore: $(CFILES) $(HFILES)
	grep -h -e '^\S.*\s.*(.*).*{' *.c | sed -e 's/{/;/g' > functiondeclarations.h
	grep '^struct\s\S*\s{' gscore.h | sed 's/^struct/typedef struct/g; s/ {//g; s/\S*$$/& &;/g' > typedeclarations.h
	$(CC) $(CFLAGS) $(INCLUDE) $(OPTS) -o gscore gscore.c $(LIBS)
	rm -f functiondeclarations.h typedeclarations.h

clean:
	rm -f gscore
