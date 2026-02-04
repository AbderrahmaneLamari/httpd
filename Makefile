CC=gcc
CFLAGS=-Wall -Wextra -Iinclude

SRC=./main.c src/server.c src/http.c src/routes.c src/clog.c src/cframework.c
OUT=httpd

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)
