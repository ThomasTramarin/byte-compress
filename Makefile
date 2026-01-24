CC = gcc
CFLAGS = -Iinclude -Wall
TARGET = build/bcomp

SRC_DIR = src
OBJ_DIR = build

# Find all C source files recursively
SRC = $(shell find $(SRC_DIR) -name '*.c')

# Map sources to object files in build/
OBJ = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $(OBJ)


# Compile .c to .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build
debug: CFLAGS += -g
debug: $(TARGET)

# Clean build files
clean:
	rm -rf $(OBJ_DIR) *.o