CC=gcc
CFLAGS=-Wall -Wextra -Iinclude
LDFLAGS=-lpthread
SRC=./main.c src/*.c
OUT=httpd

# Default build (release)
all: release

# Release build (optimized)
release: CFLAGS += -O2
release:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

# Debug build (with symbols)
debug: CFLAGS += -g -O0
debug:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

# AddressSanitizer build (detects memory errors)
asan: CFLAGS += -g -O1 -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
asan: LDFLAGS += -fsanitize=address -fsanitize=undefined
asan:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

# LeakSanitizer only (just memory leaks)
lsan: CFLAGS += -g -O1 -fsanitize=leak -fno-omit-frame-pointer
lsan: LDFLAGS += -fsanitize=leak
lsan:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

# Thread sanitizer (detects race conditions)
tsan: CFLAGS += -g -O1 -fsanitize=thread -fno-omit-frame-pointer
tsan: LDFLAGS += -fsanitize=thread
tsan:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

# Valgrind-friendly build (needs glibc-debug installed)
valgrind: CFLAGS += -g -O0
valgrind:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

clean:
	rm -f $(OUT)

# Help target
help:
	@echo "Available targets:"
	@echo "  make          - Build release version (optimized)"
	@echo "  make release  - Build release version (optimized)"
	@echo "  make debug    - Build with debug symbols"
	@echo "  make asan     - Build with AddressSanitizer (memory errors)"
	@echo "  make lsan     - Build with LeakSanitizer (memory leaks only)"
	@echo "  make tsan     - Build with ThreadSanitizer (race conditions)"
	@echo "  make valgrind - Build for Valgrind (needs glibc-debug)"
	@echo "  make clean    - Remove binary"

.PHONY: all release debug asan lsan tsan valgrind clean help