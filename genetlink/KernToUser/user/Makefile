CC = gcc
BIN = benchclient
LDFLAGS = -lnl-3 -lnl-genl-3
CFLAGS = -I/usr/include/libnl3 -Wall

all: $(BIN)

clean:
	rm *.o $(BIN)

benchclient: benchclient.o
	$(CC) $^ -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) -c $^ $(CFLAGS)
