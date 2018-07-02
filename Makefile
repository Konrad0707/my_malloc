# Makefile for CS 2110 Homework 11, Spring 2018

CC = gcc-5
CFLAGS = -std=c99 -pedantic -Wall -Werror -Wextra -g
CHECK_LIBS = $(shell pkg-config --cflags --libs check)

# The C and H files
CFILES = my_malloc.c tests.c $(patsubst %.c,%.o,$(wildcard suites/*.c))
HFILES = my_malloc.h my_sbrk.h suites/suites.h
OFILES = $(patsubst %.c,%.o,$(CFILES))

.PHONY: run-tests run-gdb run-valgrind clean

tests: $(OFILES)
	./verify.sh
	$(CC) $(CFLAGS) $^ -o $@ $(CHECK_LIBS)

%.o: %.c $(HFILES)
	$(CC) $(CFLAGS) -c $< -o $@

run-tests: tests
	./tests $(TEST)

run-gdb: tests
	CK_FORK=no gdb --args ./tests $(TEST)

clean:
	rm -rf tests $(OFILES)
