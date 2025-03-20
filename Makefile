CC=gcc

CFLAGS=

all:
	which jq >/dev/null && test $$(jq length compile_commands.json) = 0 && rm compile_commands.json || true
	[ Makefile -nt compile_commands.json ] && intercept-build make nelisp || make nelisp

nelisp:
	./lua/nelisp/makedoc.lua src/lua.c lua/nelisp/_c_meta.lua src src/globals.h src/link.c
	$(CC) src/link.c -lluajit-5.1 -shared -o nelisp.so -Wall -Wextra -Werror -pedantic $(CFLAGS) -fvisibility=hidden -fPIC -std=gnu17

format:
	clang-format src/*.c src/*.h -i --style=file

check:
	find src/ -name '*.c' -o -name '*.h' -not -name 'globals.h'|\
		xargs clang-check

analyze:
	find src/ -name '*.c' -o -name '*.h' -not -name 'globals.h'|\
		xargs clang-check --analyze --analyzer-output-path=/dev/null

.PHONY: all nelisp format check analyze
