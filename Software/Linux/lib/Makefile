lib_OBJS=pepe_rpc.o
LPATH=/usr/lib
VERSION=1
MINOR=0
BUILD=0
LNAME=libpepe_rpc.so
SONAME=$(LNAME).$(VERSION)
REALNAME=$(SONAME).$(MINOR).$(BUILD)

lib_CFLAGS=-fPIC -g -c -Wall
lib_LFLAGS=-shared -Wl,-soname,$(SONAME)

all: $(REALNAME)

$(REALNAME): $(lib_OBJS)
	gcc $(lib_LFLAGS) -o $(REALNAME) $(lib_OBJS) -lc

pepe_rpc.o: pepe_rpc.c pepe_rpc.h
	gcc $(lib_CFLAGS) pepe_rpc.c

install: all
	sudo cp $(REALNAME) $(LPATH)/
	sudo ldconfig -n $(LPATH)
	sudo ln -s $(LPATH)/$(SONAME) $(LPATH)/$(LNAME)
	sudo cp pepe_rpc.h /usr/local/include/

uninstall:
	sudo rm -f $(LPATH)/$(LNAME)
	sudo rm -f $(LPATH)/$(REALNAME)
	sudo ldconfig -n $(LPATH)
	sudo rm -f /usr/local/include/pepe_rpc.h

clean:
	rm -f $(lib_OBJS) $(REALNAME) 
