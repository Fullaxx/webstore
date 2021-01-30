# Compiling Webstore

## Server Dependencies
In order to use the client, you will need to compile against libmicrohttpd and libhiredis \
In Ubuntu, you would install build-essential libmicrohttpd-dev libhiredis-dev
```bash
apt-get install -y build-essential libmicrohttpd-dev libhiredis-dev
./compile_server.sh
```

## Run the server
XXX TODO

## Client Dependencies
In order to use the client, you will need to compile against libcurl and libgcrypt \
In Ubuntu, you would install build-essential ca-certificates libcurl4-gnutls-dev and libgcrypt20-dev
```bash
apt-get install -y build-essential ca-certificates libcurl4-gnutls-dev libgcrypt20-dev
./compile_clients.sh
```
## Run the clients
After compiling the client source code, you can run the binaries:
```bash
WSHOST="172.17.0.1"
WSPORT="80"
./ws_post.exe -H ${WSHOST} -P ${WSPORT} -v -c -f LICENSE -a 1
./ws_get.exe  -H ${WSHOST} -P ${WSPORT} -t b234ee4d69f5fce4486a80fdaf4a4263 -f LICENSE.copy
md5sum LICENSE LICENSE.copy
```
If your webstore server is running in https mode:
```bash
WSHOST="172.17.0.1"
WSPORT="443"
./ws_post.exe -s -H ${WSHOST} -P ${WSPORT} -v -c -f LICENSE -a 1
./ws_get.exe  -s -H ${WSHOST} -P ${WSPORT} -t b234ee4d69f5fce4486a80fdaf4a4263 -f LICENSE.copy
md5sum LICENSE LICENSE.copy
```
