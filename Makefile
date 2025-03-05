CC=gcc

CFLAGS=-Wall -Wextra -Werror -pedantic

all:
	./lua/nelisp/makedoc.lua src/lua.c lua/nelisp/_c_meta.lua src src/globals.h src/link.c
	$(CC) src/link.c -lluajit-5.1 -shared -o nelisp.so $(CFLAGS) -fvisibility=hidden -fPIC
