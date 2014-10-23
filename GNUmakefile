CC      = gcc
CFLAGS  = -Wall -Wextra -g -O2
LDFLAGS = -ltalloc

OBJECTS = main.o node.o path.o symlink.o tree.o children.o find.o

main: $(OBJECTS)
	gcc $(LDFLAGS) $^ -o $@

clean:
	rm -f $(OBJECTS) main

