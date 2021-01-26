#!/bin/bash

if [ -z "${WSHOST}" ]; then
  echo "Set WSHOST"
  exit 1
fi

if [ -z "${WSPORT}" ]; then
  echo "Set WSPORT"
  exit 1
fi

if [ -z "${ALG}" ]; then
  echo "Set ALG"
  exit 1
fi

if [ -z "${MSG}" ]; then
  echo "Set MSG"
  exit 1
fi

unset SECURE
if [ -n "${HTTPS}" ]; then
  SECURE="-s"
fi

if [ "${HTTPS}" == "0" ]; then unset SECURE; fi
if [ "${HTTPS}" == "false" ]; then unset SECURE; fi
if [ "${HTTPS}" == "FALSE" ]; then unset SECURE; fi

exec /app/ws_post.exe ${SECURE} -H ${WSHOST} -P ${WSPORT} -a ${ALG} -m "${MSG}"
