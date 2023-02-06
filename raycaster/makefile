CC = clang
CFLAGS  = -O3 -g -Wall -std=c17 

TARGET = raycaster

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(TARGET).c $(CFLAGS) -o $(TARGET).out

clean:
	$(RM) $(TARGET)