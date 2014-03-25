CC = gcc -O2
CFLAGS = -D_REENTRANT -Wall# -DUSE_IPV6

VPATH = src
OBJS = clone.o dcc.o link.o load.o muhstik.o print.o  \
string.o control.o init.o lists.o mass.o net.o proxy.o

.SUFFIXES: .c .o
.PHONY: muhstik

help:
	@echo "Linux:     make linux"
	@echo "Solaris:   make sun"
	@echo "FreeBSD:   make freebsd"
	@echo "NetBSD:    make netbsd"
	@echo "MacOS X:   make mac"
	@echo "Cygwin:    make cygwin"
	@echo "Amiga:     make amiga"

.c.o:
	$(CC) -c $(CFLAGS) -o src/$@ $<

muhstik: $(OBJS)
	cd src && $(CC) -o ../$@ $(OBJS) $(LDFLAGS)

telnet: telnet.c
	$(CC) -o script/telnet $<

linux:
	@make muhstik LDFLAGS=-lpthread

sun:
	@make muhstik CFLAGS="$(CFLAGS) -DNO_STRSEP" LDFLAGS="-lpthread -lsocket -lnsl"

freebsd:
	@make muhstik LDFLAGS=-pthread

netbsd:
	@make muhstik 	LDFLAGS="`pthread-config --libs` `pthread-config --ldflags`" \
			CFLAGS="$(CFLAGS) `pthread-config --cflags`"

mac: muhstik telnet

cygwin: muhstik telnet
	strip muhstik.exe script/telnet.exe

amiga:
	@make muhstik CFLAGS="$(CFLAGS) -m68020-60 -mstackextend -s" LDFLAGS=-lpthread

clean:
	cd src && rm -f *.o *~
	rm -f muhstik script/telnet.exe
