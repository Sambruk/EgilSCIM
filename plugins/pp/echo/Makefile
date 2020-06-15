CFLAGS=-I../../../src -Wall -Werror -fpic

echo.so : echo.o
	$(CC) -shared -o echo.so echo.o

echo.o : echo.c
