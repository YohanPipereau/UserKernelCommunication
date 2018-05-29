CC = gcc
CFLAGS = -O2
BIN = nlwriter nlreader

all : $(BIN)

%.o: %.c
	$(CC) -c $<

nlwriter : nlwriter.o
	$(CC) -o $@ $^

nlreader : nlreader.o
	$(CC) -o $@ $^

clean:
	rm -rf *.o $(BIN)
