CC=gcc
CFLAGS=-g -o0 -Wall -Wextra
LDFLAGS=

SRC_DIR=src
EXAMPLES_DIR=examples
OBJ_DIR=obj
BIN_DIR=bin

SRCS=$(wildcard $(SRC_DIR)/*.c)
OBJS=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
EXAMPLE_SRCS=$(wildcard $(EXAMPLES_DIR)/*.c)
EXAMPLE_BINS=$(patsubst $(EXAMPLES_DIR)/%.c, $(BIN_DIR)/%, $(EXAMPLE_SRCS))

all: $(EXAMPLE_BINS)

$(BIN_DIR)/%: $(OBJS) $(EXAMPLES_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

example: $(BIN_DIR)/$(target)

.PHONY: all clean example
