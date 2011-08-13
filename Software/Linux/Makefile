OBJS=pepe_rpc_get.o
VERSION=1
MINOR=0
BUILD=0

CFLAGS=-c -Wall

all: pepe_rpc_get

pepe_rpc_get: $(OBJS)
	gcc -o pepe_rpc_get $(OBJS) -lpepe_rpc

/usr/local/lib/pepe_rpc.so:
	echo "test"

pepe_rpc_get.o: pepe_rpc_get.c pepe_rpc_get.h
	gcc $(CFLAGS) pepe_rpc_get.c

install:

uninstall:

clean:
	rm -f $(OBJS) pepe_rpc_get
