CC = gcc
CFLAGS	= -g -Wall -O0
LDFLAGS = -lm
BIN = mmapwrapper

all: $(BIN)

mmapwrapper: mmapwrapper.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(BIN)
