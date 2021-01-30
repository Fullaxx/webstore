#!/bin/bash

set -e

HOST="172.17.0.1"
PORT="80"

if [ ! -x ./ws_post.dbg ]; then
  echo "./ws_post.dbg not found!"
  echo "Compile the clients and try again"
  exit 1
fi

if [ ! -x ./ws_get.dbg ]; then
  echo "./ws_get.dbg not found!"
  echo "Compile the clients and try again"
  exit 1
fi

rm -rf testwsget testcurl
mkdir testwsget testcurl

for FILE in *.[ch] *.sh *.exe *.dbg; do
  TOKEN=`./ws_post.dbg -H ${HOST} -P ${PORT} -a 1 -f "${FILE}" | grep 'Token: ' | awk '{print $2}'`
  ./ws_get.dbg -H ${HOST} -P ${PORT} -t "${TOKEN}" -f testwsget/${FILE}
  curl -X GET "http://${HOST}:${PORT}/store/128/${TOKEN}" -o testcurl/${FILE}.z85 2>/dev/null
done

for FILE in *.[ch] *.sh *.exe *.dbg; do
  TOKEN=`./ws_post.dbg -H ${HOST} -P ${PORT} -a 2 -f "${FILE}" | grep 'Token: ' | awk '{print $2}'`
  ./ws_get.dbg -H ${HOST} -P ${PORT} -t "${TOKEN}" -f testwsget/${FILE}
  curl -X GET "http://${HOST}:${PORT}/store/160/${TOKEN}" -o testcurl/${FILE}.z85 2>/dev/null
done

for FILE in *.[ch] *.sh *.exe *.dbg; do
  TOKEN=`./ws_post.dbg -H ${HOST} -P ${PORT} -a 3 -f "${FILE}" | grep 'Token: ' | awk '{print $2}'`
  ./ws_get.dbg -H ${HOST} -P ${PORT} -t "${TOKEN}" -f testwsget/${FILE}
  curl -X GET "http://${HOST}:${PORT}/store/224/${TOKEN}" -o testcurl/${FILE}.z85 2>/dev/null
done

for FILE in *.[ch] *.sh *.exe *.dbg; do
  TOKEN=`./ws_post.dbg -H ${HOST} -P ${PORT} -a 4 -f "${FILE}" | grep 'Token: ' | awk '{print $2}'`
  ./ws_get.dbg -H ${HOST} -P ${PORT} -t "${TOKEN}" -f testwsget/${FILE}
  curl -X GET "http://${HOST}:${PORT}/store/256/${TOKEN}" -o testcurl/${FILE}.z85 2>/dev/null
done

for FILE in *.[ch] *.sh *.exe *.dbg; do
  TOKEN=`./ws_post.dbg -H ${HOST} -P ${PORT} -a 5 -f "${FILE}" | grep 'Token: ' | awk '{print $2}'`
  ./ws_get.dbg -H ${HOST} -P ${PORT} -t "${TOKEN}" -f testwsget/${FILE}
  curl -X GET "http://${HOST}:${PORT}/store/384/${TOKEN}" -o testcurl/${FILE}.z85 2>/dev/null
done

for FILE in *.[ch] *.sh *.exe *.dbg; do
  TOKEN=`./ws_post.dbg -H ${HOST} -P ${PORT} -a 6 -f "${FILE}" | grep 'Token: ' | awk '{print $2}'`
  ./ws_get.dbg -H ${HOST} -P ${PORT} -t "${TOKEN}" -f testwsget/${FILE}
  curl -X GET "http://${HOST}:${PORT}/store/512/${TOKEN}" -o testcurl/${FILE}.z85 2>/dev/null
done
