CC     = gcc
C_FILE = $(wildcard *.c)
TARGET = $(patsubst %.c,%,$(C_FILE))
CFLAGS = -O3 -Wall -Werror -pedantic-errors 

all:
	$(CC) $(CFLAGS) $(C_FILE) -o $(TARGET) -lm -pthread
clean:
	rm -f $(TARGET) $(TARGET).exe
