CC = clangs
CFLAGS  = -O3 -g -Wall -std=c17 

TARGET = gameofwar

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(TARGET).c $(CFLAGS) -o $(TARGET) 

clean:
	$(RM) $(TARGET)