# MidnightBSD mport gui
CC=clang
CFLAGS= -I/usr/include -I/usr/local/include -Wall -pedantic -std=c99 -O2 `pkg-config --cflags gtk+-3.0` \
	-DDATADIR="\"${DATADIR}\""
LDFLAGS= -L/usr/lib -L/usr/local/lib \
	-lmd -larchive -lbz2 -llzma -lz -lfetch -lsqlite3 -lmport -lutil \
	-lpthread \
	`pkg-config --libs gtk+-3.0` -lX11
PREFIX=	/usr/local
DATADIR=/usr/local/share/mport

all: clean mport-manager

mport-manager: mport-manager.c
	${CC} ${CFLAGS} ${LDFLAGS} -o mport-manager mport-manager.c

install:
	install mport-manager ${DESTDIR}${PREFIX}/bin/mport-manager
	mkdir -p ${DESTDIR}${DATADIR}
	install -m 444 icon.png ${DESTDIR}${DATADIR}/icon.png
	install -m 444 icon.png ${DESTDIR}${PREFIX}/share/icons/hicolor/48x48/apps/mport-manager.png
	install -m 444 mport-manager.desktop ${DESTDIR}${PREFIX}/share/applications/

clean:
	rm -f *.o mport-manager
