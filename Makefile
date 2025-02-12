# CC=gcc
CC=clang

CFLAGS=-Wall -Wextra -Werror -pedantic

all:
	$(CC) src/lua.c -lluajit-5.1 -shared -o nelisp.so $(CFLAGS) -fvisibility=hidden
