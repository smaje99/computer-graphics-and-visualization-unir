CC := gcc
TARGET := primitivas_opengl
SRC := primitivas_opengl.c

CFLAGS := -Wall -Wextra -std=c11
LDLIBS := -lglut -lGLU -lGL -lm

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDLIBS)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
