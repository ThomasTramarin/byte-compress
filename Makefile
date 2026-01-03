CC = gcc
CFLAGS = -Iinclude -Wall -g
TARGET = build/bcomp

SRC = src/main.c src/cli.c src/rle.c

$(TARGET):	$(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -rf build