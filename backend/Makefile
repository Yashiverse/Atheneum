# Makefile for Library Management System C Core

CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = library_core.exe
TARGET_NIX = library_core

all: $(TARGET)

$(TARGET): library_core.c library_core.h
	$(CC) $(CFLAGS) library_core.c -o $(TARGET)

nix: library_core.c library_core.h
	$(CC) $(CFLAGS) library_core.c -o $(TARGET_NIX)

clean:
	rm -f $(TARGET) $(TARGET_NIX) data/*.dat backend/data/*.dat

.PHONY: all nix clean
