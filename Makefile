CC=gcc
CFLAGS=-std=c99 -O3 -pedantic-errors -Wall -Wextra -Werror=vla -DGLEW_NO_GLU
CFILES=canvas.c config.h gscore.c gscore.h input.c main.c note.c renderer.c util.c

LIBS=-lGL -lGLEW -lglfw

gscore: $(CFILES)
	$(CC) $(CFLAGS) -o gscore gscore.c $(LIBS)

clean:
	rm -f gscore
