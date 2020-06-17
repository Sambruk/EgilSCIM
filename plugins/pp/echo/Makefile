CFLAGS=-I../../../src -Wall -Werror -fpic

libecho.so : echo.o
	$(CC) -shared -o libecho.so echo.o

echo.o : echo.c
