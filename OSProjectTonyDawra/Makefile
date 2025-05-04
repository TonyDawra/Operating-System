# Declare the default target 'all' with 'main' as its dependency
all: main

# Compiler and compiler flags
CC = clang
override CFLAGS += -g -Wno-everything -pthread -lm

# Find all source files and header files in the current directory and its subdirectories
SRCS = $(shell find . -name '.ccls-cache' -type d -prune -o -type f -name '*.c' -print)
HEADERS = $(shell find . -name '.ccls-cache' -type d -prune -o -type f -name '*.h' -print)

# Rule to build the 'main' target
main: $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) $(SRCS) -o "$@"

# Rule to build the 'main-debug' target with debugging symbols
main-debug: $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -O0 $(SRCS) -o "$@"

# Rule to clean the generated files
clean:
	rm -f main main-debug
