CC = gcc
CFLAGS = -O2
BIN = nlwriter

all : $(BIN)

%.o: %.c
	$(CC) -c $<

nlwriter : nlwriter.o
	$(CC) -o $@ $^

clean:
	rm -rf *.o $(BIN)
