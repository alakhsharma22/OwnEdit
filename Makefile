CC = cc
CFLAGS = -std=c11 -Wall -Wextra -pedantic -O2
LDFLAGS =
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
BIN = ownedit

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LDFLAGS)

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean