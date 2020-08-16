CC=gcc
CFLAGS=-std=c99 -O3 -pedantic-errors -Wall -Wextra -DGLEW_NO_GLU
CFILES=gscore.c
LIBS=-lGL -lGLEW -lglfw

gscore: $(CFILES)
	$(CC) $(CFLAGS) -o gscore $(CFILES) $(LIBS)

clean:
	rm -f gscore
