#
# Makefile for mount_emptyfs
#

CC=gcc
CFLAGS=-std=c99 -Wall -Wextra -g -DDEBUG
SOURCES=$(wildcard *.c)
EXECUTABLE=mount_emptyfs
RM=rm -f

all: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) *.o $(EXECUTABLE)

.PHONY: all clean

