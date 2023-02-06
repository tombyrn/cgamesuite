# the compiler: gcc for C program, define as g++ for C++
CC = clang

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -O3 -std=c17 

# the build target executable:
TARGET = cwordlesolver

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(TARGET).c $(CFLAGS) -o $(TARGET) 

clean:
	$(RM) $(TARGET)