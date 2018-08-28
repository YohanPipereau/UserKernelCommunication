CC = gcc
BIN = benchclient
PWD = $(shell pwd)
LDFLAGS = -lnl-3 -lnl-genl-3 -g
CFLAGS = -I/usr/include/libnl3 -I$(PWD)/.. -Wall -g

all: $(BIN)

clean:
	rm *.o $(BIN) || true

benchclient: benchclient.o benchclient-api.o
	$(CC) $^ -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) -c $^ $(CFLAGS)
