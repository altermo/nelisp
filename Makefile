CC=gcc

CFLAGS=-Wall -Wextra -Werror -pedantic

# This makes compiling with `zig cc` not crash.
ifeq ($(CC),zig cc)
	CFLAGS+=-O1
endif

all:
	./lua/nelisp/makedoc.lua src/lua.c lua/nelisp/_c_meta.lua src src/globals.h
	$(CC) src/lua.c -lluajit-5.1 -shared -o nelisp.so $(CFLAGS) -fvisibility=hidden -fPIC
