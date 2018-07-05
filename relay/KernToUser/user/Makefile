CC = gcc
CFLAGS	= -g -Wall -O0 -fopenmp
LDFLAGS = -lm -lgomp
BIN = relaywrapper

all: $(BIN)

relaywrapper: relaywrapper.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(BIN)
