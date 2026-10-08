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
	rm -rf build tests/test_cc tests/test_real ccdump
.PHONY: test clean

ccdump: ccdump.c $(CORE) cc.h maze.h
	$(CC) $(CFLAGS) -o $@ ccdump.c $(CORE)

# compare the C extraction with tools/mm3_cc.py on real archives (DATA = folder with MM3.CC / MM3.CUR)
DATA ?= ../data
crosscheck: ccdump
	@for a in MM3.CC MM3.CUR; do \
	  rm -rf build/c build/py; mkdir -p build/c build/py; \
	  ./ccdump $(DATA)/$$a build/c && python3 ../tools/mm3_cc.py extract $(DATA)/$$a build/py && \
	  diff -rq build/c build/py && echo "$$a: identical ($$(ls build/c | wc -l) members)"; \
	done
.PHONY: crosscheck

tests/test_real: tests/test_real.c $(CORE) cc.h maze.h
	$(CC) $(CFLAGS) -o $@ tests/test_real.c $(CORE)

realtest: tests/test_real
	tests/test_real $(DATA)/MM3.CUR
.PHONY: realtest
