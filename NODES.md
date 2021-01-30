# Webstore Nodes
This document aims to describe the webstore server nodes

## Choosing an Algorithm
Each webstore server instance has 6 nodes (defined in webstore_uhd.c) to GET/POST data to:
```
/store/128/
/store/160/
/store/224/
/store/256/
/store/384/
/store/512/
```
Each node accepts a token of specific length to use as storage UUID \
Currently there are 6 algorithm types (defined in webstore.h) to choose from:
```
-a 1 (128 bits)
-a 2 (160 bits)
-a 3 (224 bits)
-a 4 (256 bits)
-a 5 (384 bits)
-a 6 (512 bits)
```
These algorithm types default to the following algorithms (defined in gchash.h):
```
128 bits - md5
160 bits - sha1
224 bits - sha224
256 bits - sha256
384 bits - sha384
512 bits - sha512
```

## URL Creation
Examples of proper URL creation (handled in webstore_url.h) are shown here:
```bash
WSHOST="172.17.0.1"
WSPORT="80"
./ws_post.exe -H ${WSHOST} -P ${WSPORT} -f LICENSE -a 1
./ws_get.exe  -H ${WSHOST} -P ${WSPORT} -t b234ee4d69f5fce4486a80fdaf4a4263
http://172.17.0.1:80/store/128/b234ee4d69f5fce4486a80fdaf4a4263

WSHOST="172.17.0.1"
WSPORT="443"
./ws_post.exe -s -H ${WSHOST} -P ${WSPORT} -f LICENSE -a 4
./ws_get.exe  -s -H ${WSHOST} -P ${WSPORT} -t 8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643
https://172.17.0.1:443/store/256/8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643
```
