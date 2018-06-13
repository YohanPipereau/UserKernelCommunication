CC = gcc
CFLAGS = -O2
LDFLAGS =
BIN = writewrapper

all: $(BIN)

writewrapper: writewrapper.c
	$(CC) -o $@ $^ $(LDFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean :
	rm -f *.o $(BIN)
