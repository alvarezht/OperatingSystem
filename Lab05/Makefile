CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -pedantic
TARGET = log_analyzer_linux
SRCS = main_linux.c log_processor.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRCS) log_processor.h
	$(CC) $(CFLAGS) -pthread $(SRCS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET) access.log 4

clean:
	rm -f $(TARGET)
