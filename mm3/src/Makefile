# Might and Magic III C reimplementation -- core (no SDL yet)
CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -O2

CORE = cc.c maze.c

test: tests/test_cc
	python3 tests/gen_testdata.py build/testdata
	tests/test_cc build/testdata

tests/test_cc: tests/test_cc.c $(CORE) cc.h maze.h
	$(CC) $(CFLAGS) -o $@ tests/test_cc.c $(CORE)

clean:
	rm -rf build tests/test_cc
.PHONY: test clean
