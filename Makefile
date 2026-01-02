CC = gcc
CFLAGS = -Iinclude -Wall
TARGET = build/bcomp

SRC = src/main.c

$(TARGET):
	mkdir -p build
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -rf build