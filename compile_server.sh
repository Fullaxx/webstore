#!/bin/bash

set -e

OPT="-O2"
DBG="-ggdb3 -DDEBUG"
CFLAGS="-Wall"
OPTCFLAGS="${CFLAGS} ${OPT}"
DBGCFLAGS="${CFLAGS} ${DBG}"

rm -f *.exe *.dbg

gcc ${OPTCFLAGS} webstore*.c getopts.c searest*.c rai.c futils.c \
-lpthread -lmicrohttpd -lhiredis -o webstore.exe

gcc ${DBGCFLAGS} webstore*.c getopts.c searest*.c rai.c futils.c \
-lpthread -lmicrohttpd -lhiredis -o webstore.dbg

strip *.exe
