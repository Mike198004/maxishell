# Makefile for maxishell
CC = gcc
CFLAGS = -O2 -g -Werror
LDFLAGS =

PROGRAMS = \
	maxishell \

all: $(PROGRAMS)

.c:
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	-rm -f $(PROGRAMSS)
