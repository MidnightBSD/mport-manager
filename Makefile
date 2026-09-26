# MidnightBSD mport gui
CC?=clang
CFLAGS= -I. -Itests/mocks -I/usr/local/include -Wall -pedantic -std=c99 -O2 `pkg-config --cflags gtk4` \
        -DDATADIR="\"${DATADIR}\""
LDFLAGS= -Ltests/mocks -L/usr/lib -L/usr/local/lib \
        -lmd -larchive -lbz2 -llzma -lz -lfetch -lsqlite3 -lmport -lutil \
        -lpthread \
        `pkg-config --libs gtk4`

PREFIX=	/usr/local
DATADIR=/usr/local/share/mport

all: clean mport-manager

tests/mocks/libmport.a: tests/mocks/stubs.c
	${CC} ${CFLAGS} -c tests/mocks/stubs.c -o tests/mocks/stubs.o
	ar rcs tests/mocks/libmport.a tests/mocks/stubs.o
	ar rcs tests/mocks/libfetch.a tests/mocks/stubs.o

mport-manager: mport-manager.c tests/mocks/libmport.a
	${CC} ${CFLAGS} -o mport-manager mport-manager.c tests/mocks/stubs.c ${LDFLAGS}

tests/progress_test: tests/progress_test.c mport-manager.c tests/mocks/libmport.a
	${CC} ${CFLAGS} -DMPORT_MANAGER_TESTING tests/progress_test.c tests/mocks/stubs.c mport-manager.c `pkg-config --libs gtk4` -latf-c -lm -o tests/progress_test

test: tests/progress_test
	kyua test -k Kyuafile

install:
	mkdir -p ${DESTDIR}${PREFIX}/bin
	install mport-manager ${DESTDIR}${PREFIX}/bin/mport-manager
	mkdir -p ${DESTDIR}${DATADIR}
	install -m 444 icon.png ${DESTDIR}${DATADIR}/icon.png
	mkdir -p ${DESTDIR}${PREFIX}/share/icons/hicolor/48x48/apps
	install -m 444 icon.png ${DESTDIR}${PREFIX}/share/icons/hicolor/48x48/apps/mport-manager.png
	mkdir -p ${DESTDIR}${PREFIX}/share/applications
	install -m 444 mport-manager.desktop ${DESTDIR}${PREFIX}/share/applications/
	mkdir -p ${DESTDIR}${PREFIX}/share/polkit-1/actions/
	install -m 444 org.midnightbsd.mport-manager.policy ${DESTDIR}${PREFIX}/share/polkit-1/actions/

clean:
	rm -f *.o tests/mocks/*.o tests/mocks/*.a mport-manager tests/progress_test
