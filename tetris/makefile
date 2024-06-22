CC = clang
CFLAGS  = -O3 -g -Wall -Werror -std=c17

TARGET = tetris

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(TARGET).c $(CFLAGS) -o $(TARGET).out

clean:
	$(RM) $(TARGET)