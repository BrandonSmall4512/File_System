CC = gcc
CFLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant -I./
SRC = userfs.c test_userfs.c
OBJ = $(SRC:.c=.o)
EXEC = test
TEST_MODE = -DTEST_MODE

all: $(EXEC)

$(EXEC): userfs.o test_userfs.o
	$(CC) $(CFLAGS) userfs.o test_userfs.o -o $(EXEC)

test: CFLAGS += $(TEST_MODE)
test: $(EXEC)

userfs.o: userfs.c
	$(CC) $(CFLAGS) -c userfs.c -o userfs.o

test_userfs.o: test_userfs.c
	$(CC) $(CFLAGS) $(TEST_MODE) -c test_userfs.c -o test_userfs.o

clean:
	rm -f $(OBJ) $(EXEC)

.PHONY: all clean test

