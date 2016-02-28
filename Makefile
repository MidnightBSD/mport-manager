# MidnightBSD mport gui
CC=clang
CFLAGS= -I/usr/include -I/usr/local/include -Wall -pedantic -std=c99 -O2 -fblocks `pkg-config --cflags gtk+-3.0` \
	-DDATADIR="\"${DATADIR}\""
LDFLAGS= -L/usr/lib -L/usr/local/lib \
	-lmd -larchive -lbz2 -llzma -lz -lfetch -lsqlite3 -lmport \
	-lpthread -ldispatch -lBlocksRuntime \
	`pkg-config --libs gtk+-3.0`
PREFIX=	/usr/local
DATADIR=/usr/local/share/mport

all: clean mport-manager

mport-manager: mport-manager.c
	${CC} ${CFLAGS} ${LDFLAGS} -o mport-manager mport-manager.c

install:
	install mport-manager ${DESTDIR}${PREFIX}/bin/mport-manager
	mkdir -p ${DESTDIR}${DATADIR}
	install icon.png ${DESTDIR}${DATADIR}/icon.png

clean:
	rm -f *.o mport-manager
