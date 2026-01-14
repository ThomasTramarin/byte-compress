CC = gcc
CFLAGS = -Iinclude -Wall
TARGET = build/bcomp

SRC = src/main.c src/cli.c src/rle.c src/crc.c src/bcomp.c src/parser.c src/cmd/compress/compress.c src/errors.c

$(TARGET):	$(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

debug:
	mkdir -p build
	$(CC) $(CFLAGS) -g -o $(TARGET) $(SRC)

clean:
	rm -rf build