CC = gcc
BIN = nlreader
CFLAGS = -O2 -g

all : $(BIN)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

nlreader: nlreader.o
	$(CC) -o $@ $^

clean:
	rm -rf *.o $(BIN)
