CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = vat_sim
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
