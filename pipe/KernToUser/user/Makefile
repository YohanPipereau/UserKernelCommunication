CC = gcc
CFLAGS	= -g -Wall -O0
LDFLAGS = -lm
BIN = pipeus

all: $(BIN)

pipeus: pipeus.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(BIN)


