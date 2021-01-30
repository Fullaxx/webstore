#!/bin/bash

set -e

CURLCFLAGS=`curl-config --cflags`
CURLLIBS=`curl-config --libs`
GCCFLAGS=`libgcrypt-config --cflags`
GCLIBS=`libgcrypt-config --libs`

OPT="-O2"
DBG="-ggdb3 -DDEBUG"
CFLAGS="-Wall ${CURLCFLAGS} ${GCCFLAGS}"
CFLAGS+=" -DMINIZ_COMPRESSION"
OPTCFLAGS="${CFLAGS} ${OPT}"
DBGCFLAGS="${CFLAGS} ${DBG}"

rm -f *.exe *.dbg

gcc ${OPTCFLAGS} ws_get.c curl_ops.c z85.c compression.c getopts.c \
${CURLLIBS} -o ws_get.exe

gcc ${DBGCFLAGS} ws_get.c curl_ops.c z85.c compression.c getopts.c \
${CURLLIBS} -o ws_get.dbg

gcc ${OPTCFLAGS} ws_post.c futils.c gchash.c curl_ops.c z85.c compression.c getopts.c \
${CURLLIBS} ${GCLIBS} -o ws_post.exe

gcc ${DBGCFLAGS} ws_post.c futils.c gchash.c curl_ops.c z85.c compression.c getopts.c \
${CURLLIBS} ${GCLIBS} -o ws_post.dbg

strip *.exe
