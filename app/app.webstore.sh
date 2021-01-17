#!/bin/bash

HTTPPORT=${HTTPPORT:-8080}
REDISIP=${REDISIP:-127.0.0.1}
REDISPORT=${REDISPORT:-6379}

exec /app/webstore.exe -P ${HTTPPORT} --rtcp ${REDISIP}:${REDISPORT} -l /log/webstore.log
