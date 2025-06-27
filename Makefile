CC=gcc

WFLAGS= -Wall -Wextra -Werror -pedantic
WEFLAGS= -Wimplicit-fallthrough -Wno-sign-compare -Wno-unused-variable -Wno-unused-parameter
LFLAGS= -lgmp -lluajit-5.1 -I./lib
OFLAGS= -fvisibility=hidden -fPIC -std=gnu17 -fno-sanitize=undefined
CFLAGS= $(WFLAGS) $(WEFLAGS) $(LFLAGS) $(OFLAGS)

SOURCES := $(wildcard src/*.c)

all:
	which jq >/dev/null && test $$(jq length compile_commands.json) = 0 && rm compile_commands.json || true
	[ Makefile -nt compile_commands.json ] && intercept-build make nelisp || make nelisp.so

fast: src/globals.h
	echo $(SOURCES)|sed 's/src\/\([^ ]*\)/\n#include "\1"/g'|$(CC) -xc - $(CFLAGS) -I./src -shared -o nelisp.so

nelisp.so: src/globals.h $(SOURCES)
	make nelisp

nelisp: src/globals.h
	$(CC) $(SOURCES) $(CFLAGS) -shared -o nelisp.so

src/globals.h: $(SOURCES)
	make doc

doc:
	./lua/nelisp/scripts/makedoc.lua src/lua.c lua/nelisp/_c_meta.lua src src/globals.h

format:
	clang-format src/*.c src/*.h -i --style=file

check:
	find src/ -name '*.c' -o -name '*.h' -not -name 'globals.h'|\
		xargs clang-check

analyze:
	find src/ -name '*.c' -o -name '*.h' -not -name 'globals.h'|\
		xargs clang-check --analyze --analyzer-output-path=/dev/null --extra-arg=-Dlint

.PHONY: all nelisp format check analyze doc
