CC?=cc
#CFLAGS?=-O2
CFLAGS?=-Og -g

PREFIX=/usr/local

VERSION = 0.1.0-git

CFILES=application.c block.c editview.c gscore.c hashmap.c input.c main.c objectview.c renderer.c score.c synth.c track.c util.c xevents.c
HFILES=config.h gscore.h
INCLUDE=$$(xml2-config --cflags)
LIBS=-lGL -lGLEW -lglfw -lfluidsynth -lm -lX11 $$(xml2-config --libs)

WARNINGS=-Wall -Wextra -Wunused-const-variable -pedantic
ERRORS=-Werror=vla -Werror=implicit-fallthrough -Werror=strict-prototypes -Wfatal-errors
DEFINES=-DVERSION=\"${VERSION}\" -DGLEW_NO_GLU -DGLFW_EXPOSE_NATIVE_X11 -D_POSIX_C_SOURCE=200809L
OPTS=-std=c99 $(WARNINGS) $(ERRORS) $(DEFINES)

gscore: $(CFILES) $(HFILES) fileformatschema.h functiondeclarations.h typedeclarations.h
	$(CC) $(CFLAGS) $(INCLUDE) $(OPTS) -o $@ gscore.c $(LIBS)

fileformatschema.h: fileformatschema.xsd
	sed 's/"/\\"/g' $^ | sed -e 's/.*/"&\\n"/' | sed '1s/^/const char* const FILE_FORMAT_SCHEMA = \n/' | sed '$$s/$$/;\n/' > fileformatschema.h

functiondeclarations.h: $(CFILES)
	grep -h -e '^static\s\S*\s.*(.*)\s{$$' $^ | sed -e 's/{/;/g' > functiondeclarations.h

typedeclarations.h: gscore.h
	grep '^struct\s\S*\s{$$' $^ | sed 's/^struct/typedef struct/g; s/ {//g; s/\S*$$/& &;/g' > typedeclarations.h

.PHONY: clean
clean:
	rm -f gscore functiondeclarations.h typedeclarations.h fileformatschema.h

.PHONY: install
install: gscore gscore-export-midi.py
	install -Dm 755 gscore "${DESTDIR}${PREFIX}/bin/gscore"
	install -Dm 755 gscore-export-midi.py "${DESTDIR}${PREFIX}/bin/gscore-export-midi"

.PHONY: uninstall
uninstall:
	rm -f "${DESTDIR}${PREFIX}/bin/gscore"
	rm -f "${DESTDIR}${PREFIX}/bin/gscore-export-midi"

.PHONY: cppcheck
cppcheck:
	cppcheck --enable=all gscore.c

.PHONY: black
black:
	black --check --line-length 120 gscore-export-midi.py
