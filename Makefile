CC=gcc
CFLAGS=-Wall -Wextra -Iinclude

SRC=./main.c src/*.c
OUT=httpd

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)
