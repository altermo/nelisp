CC=gcc

CFLAGS=

all:
	which jq >/dev/null && test $$(jq length compile_commands.json) = 0 && rm compile_commands.json || true
	[ Makefile -nt compile_commands.json ] && intercept-build make nelisp || make nelisp

nelisp:
	./lua/nelisp/makedoc.lua src/lua.c lua/nelisp/_c_meta.lua src src/globals.h src/link.c
	$(CC) src/link.c -lluajit-5.1 -shared -o nelisp.so -Wall -Wextra -Werror -pedantic $(CFLAGS) -fvisibility=hidden -fPIC -std=c17

.PHONY: all nelisp
