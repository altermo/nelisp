# CC=gcc
CC=clang

CFLAGS=-Wall -Wextra -Werror -pedantic

all:
	./lua/nelisp/makedoc.lua src/lua.c lua/nelisp/_c_meta.lua src src/globals.h
	$(CC) src/lua.c -lluajit-5.1 -shared -o nelisp.so $(CFLAGS) -fvisibility=hidden
