CC=gcc

CFLAGS=-Wall -Wextra -Werror -pedantic

all:
	$(CC) src/lua.c -lluajit-5.1 -shared -o nelisp.so $(CFLAGS)
