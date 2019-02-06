#
# Makefile for mount_emptyfs
#

CC=gcc
CFLAGS=-std=c99 -Wall -Wextra
SOURCES=$(wildcard *.c)
EXECUTABLE=mount_emptyfs
RM=rm

all: debug

release: $(EXECUTABLE)

debug: CFLAGS += -g -DDEBUG
debug: release

$(EXECUTABLE): $(SOURCES)
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) -rf *.o $(EXECUTABLE) *.dSYM

.PHONY: all debug release clean

