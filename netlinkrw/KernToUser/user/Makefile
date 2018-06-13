CC = gcc
CFLAGS = -O2
BIN = nlreader

all : $(BIN)

%.o: %.c
	$(CC) -c $<

nlreader: nlreader.o
	$(CC) -o $@ $^

clean:
	rm -rf *.o $(BIN)
