#!/bin/bash

HTTPPORT=${HTTPPORT:-8080}
REDISIP=${REDISIP:-127.0.0.1}
REDISPORT=${REDISPORT:-6379}

unset CERTPATH
unset KEYPATH
unset CERTARG
unset KEYARG
if [ -n "${CERTFILE}" ] && [ -n "${KEYFILE}" ]; then
  CERTPATH="/cert/${CERTFILE}"
  KEYPATH="/cert/${KEYFILE}"
  if [ ! -r "${CERTPATH}" ]; then
    echo "${CERTPATH} is not readable!"
    exit 1
  fi
  if [ ! -r "${KEYPATH}" ]; then
    echo "${KEYPATH} is not readable!"
    exit 1
  fi
  CERTARG="--cert ${CERTPATH}"
  KEYARG="--key ${KEYPATH}"
fi

unset DSIZEARG
if [ -n "${MAXPOSTSIZE}" ]; then
  DSIZEARG="--dsize ${MAXPOSTSIZE}"
fi

exec /app/webstore.exe -P ${HTTPPORT} \
--rtcp ${REDISIP}:${REDISPORT} \
-l /log/webstore.log \
${CERTARG} ${KEYARG} ${DSIZEARG}
