CC=gcc

CFLAGS=-Wall -Wextra -Werror -pedantic

all:
	$(CC) src/lua.c src/luaf.c -lluajit-5.1 -shared -o nelisp.so $(CFLAGS) -fvisibility=hidden
