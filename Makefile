CC=gcc
CFLAGS=-Wall -Wextra -Iinclude

SRC=src/main.c src/server.c src/http.c src/routes.c
OUT=httpd

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)
