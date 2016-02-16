# MidnightBSD mport gui
CC=clang
CFLAGS= -I/usr/local/include -Wall -pedantic -std=c99 -O2 -fblocks `pkg-config --cflags gtk+-3.0`
LDFLAGS= -L/usr/local/lib \
	-lmd -larchive -lbz2 -llzma -lz -lfetch -lsqlite3 -lmport \
	-lpthread -ldispatch -lBlocksRuntime \
	`pkg-config --libs gtk+-3.0`
PREFIX=	/usr/local

all: clean mport-manager

gtkjj: mport-manager.c
	${CC} ${CFLAGS} ${LDFLAGS} -o mport-manager mport-manager.c

install:
	install mport-manager ${DESTDIR}${PREFIX}/bin/mport-manager

clean:
	rm -f *.o mport-manager
