# MidnightBSD mport gui
CC?=clang
CFLAGS= -I/usr/include -I/usr/local/include -Wall -pedantic -std=c99 -O2 `pkg-config --cflags gtk4` \
        -DDATADIR="\"${DATADIR}\""
LDFLAGS= -L/usr/lib -L/usr/local/lib \
        -lmd -larchive -lbz2 -llzma -lz -lfetch -lsqlite3 -lmport -lutil \
        -lpthread \
        `pkg-config --libs gtk4`

PREFIX=	/usr/local
DATADIR=/usr/local/share/mport

all: clean mport-manager

mport-manager: mport-manager.c mport-progress.c
	${CC} ${CFLAGS} ${LDFLAGS} -o mport-manager mport-manager.c mport-progress.c

tests/reset_progress_bar_test: tests/reset_progress_bar_test.c mport-progress.c
	${CC} -Wall -std=c99 -O2 -I. -DTEST_SUITE `pkg-config --cflags atf-c` -o tests/reset_progress_bar_test tests/reset_progress_bar_test.c mport-progress.c `pkg-config --libs atf-c`

test: tests/reset_progress_bar_test
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
	rm -f *.o mport-manager tests/reset_progress_bar_test
