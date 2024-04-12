CC?=cc
#CFLAGS?=-O2
CFLAGS?=-Og -g

PREFIX=/usr/local

VERSION = 0.2.0-git

INCLUDE=$$(xml2-config --cflags) -Isrc
LIBS=-lGL -lGLEW -lglfw -lfluidsynth -lm -lX11 $$(xml2-config --libs)

WARNINGS=-Wall -Wextra -pedantic
ERRORS=-Werror=vla -Werror=implicit-fallthrough -Werror=strict-prototypes -Wfatal-errors
DEFINES=-DMAKEFILE_DEFINED_VERSION=\"${VERSION}\" -DGLEW_NO_GLU -DGLFW_EXPOSE_NATIVE_X11 -D_POSIX_C_SOURCE=200809L
OPTS=-std=c99 $(WARNINGS) $(ERRORS) $(DEFINES)

OBJS=\
	src/application/application.o \
	src/common/constants/fluidmidi.o \
	src/common/constants/input.o \
	src/common/score/score.o \
	src/common/util/alloc.o \
	src/common/util/colors.o \
	src/common/util/inputmatcher.o \
	src/common/util/hash.o \
	src/common/util/hashmap.o \
	src/common/util/hashset.o \
	src/common/util/log.o \
	src/common/util/math.o \
	src/common/util/stringmap.o \
	src/common/util/stringset.o \
	src/common/visual/grid/grid.o \
	src/events/events.o \
	src/main/main.o \
	src/ui/editview/editview.o \
	src/ui/objectview/objectview.o \
	src/window/renderer.o \
	src/window/renderwindow.o \
	src/synth/synth.o

gscore: $(OBJS)
	@$(CC) $(CFLAGS) $(INCLUDE) $(OPTS) -o $@ $(OBJS) $(LIBS)

-include $(OBJS:.o=.d)

%.o: %.c
	@$(CC) -MD $(CFLAGS) $(INCLUDE) $(OPTS) -o $@ -c $<

.PHONY: clean
clean:
	@rm -f gscore
	@find * -name "*.d" | xargs rm -f
	@find * -name "*.o" | xargs rm -f

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
	cppcheck --enable=all --suppress=missingIncludeSystem -Isrc $(OBJS:.o=.c)

.PHONY: black
black:
	black --check --line-length 120 src/export/gscore-export-midi.py
