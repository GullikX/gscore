CC?=cc
#CFLAGS?=-O2
CFLAGS?=-Og -g

VERSION = 0.0.1-git

CFILES=application.c block.c editview.c gscore.c hashmap.c input.c main.c objectview.c renderer.c score.c synth.c track.c util.c xevents.c
HFILES=config.h gscore.h
INCLUDE=$$(xml2-config --cflags)
LIBS=-lGL -lGLEW -lglfw -lfluidsynth -lm -lX11 $$(xml2-config --libs)

WARNINGS=-Wall -Wextra -Wunused-const-variable -pedantic
ERRORS=-Werror=vla -Werror=implicit-fallthrough -Werror=strict-prototypes -Wfatal-errors
DEFINES=-DVERSION=\"${VERSION}\" -DGLEW_NO_GLU -DGLFW_EXPOSE_NATIVE_X11 -D_POSIX_SOURCE
OPTS=-std=c99 $(WARNINGS) $(ERRORS) $(DEFINES)

gscore: $(CFILES) $(HFILES) fileformatschema.xsd
	grep -h -e '^static\s\S*\s.*(.*)\s{$$' *.c | sed -e 's/{/;/g' > functiondeclarations.h
	grep '^struct\s\S*\s{$$' gscore.h | sed 's/^struct/typedef struct/g; s/ {//g; s/\S*$$/& &;/g' > typedeclarations.h
	sed 's/"/\\"/g' fileformatschema.xsd | sed -e 's/.*/"&\\n"/' | sed '1s/^/const char* const FILE_FORMAT_SCHEMA = \n/' | sed '$$s/$$/;\n/' > fileformatschema.h
	$(CC) $(CFLAGS) $(INCLUDE) $(OPTS) -o gscore gscore.c $(LIBS)
	rm -f functiondeclarations.h typedeclarations.h fileformatschema.h

.PHONY: clean
clean:
	rm -f gscore

.PHONY: cppcheck
cppcheck:
	cppcheck --enable=all gscore.c
