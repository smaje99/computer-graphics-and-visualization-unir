CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -pedantic -O2
LDFLAGS := -lX11 -lGL -lm
TARGET := figura_1
SOURCE := main.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET)
