# webstore
A web-based arbitrary data storage service

## Base Docker Image
[Ubuntu](https://hub.docker.com/_/ubuntu) 20.04 (x64)

## Software
[libmicrohttpd](https://www.gnu.org/software/libmicrohttpd/) \
[libgcrypt](https://gnupg.org/software/libgcrypt/index.html) \
[redis](https://redis.io/)

## Get the image from Docker Hub
```
docker pull fullaxx/webstore
docker pull fullaxx/webstore-get
docker pull fullaxx/webstore-post
```

## Build it locally using the github repository
```
docker build -f Dockerfile.webstore -t="fullaxx/webstore" github.com/Fullaxx/webstore
docker build -f Dockerfile.wsget -t="fullaxx/webstore-get" github.com/Fullaxx/webstore
docker build -f Dockerfile.wspost -t="fullaxx/webstore-post" github.com/Fullaxx/webstore
```

## Volume Options
webstore will create a log file in /log/ \
Use the following volume option to expose it
```
-v /srv/docker/webstore/log:/log
```

## Server Instructions
Start redis at 172.17.0.1:6379 \
Bind webstore to 51.195.74.100:80 \
Save log file to /srv/docker/webstore/log
```
docker run -d --rm --name redis -p 172.17.0.1:6379:6379 redis
docker run -d --name webstore -e REDISIP=172.17.0.1 -p 51.195.74.100:80:8080 -v /srv/docker/webstore/log:/log fullaxx/webstore
```
Start redis at 172.17.0.1:7777 \
Bind webstore to 51.195.74.100:80
```
docker run -d --rm --name redis -p 172.17.0.1:7777:6379 redis
docker run -d --name webstore -e REDISIP=172.17.0.1 -e REDISPORT=7777 -p 51.195.74.100:80:8080 fullaxx/webstore
```

## Server Configuration
The default POST upload data limit is 20 MiB or (20\*1024\*1024) \
You can set this to any value with the MAXPOSTSIZE environment variable
```
-e MAXPOSTSIZE=1000000
```
You can enable connection limiting on a per IP basis by setting 2 environment variables \
REQPERIOD=1 REQCOUNT=10 will allow each IP address 10 connections within a 1 second window \
REQPERIOD=2 REQCOUNT=15 will allow each IP address 15 connections within a 2 second window \
Connection limiting will only be enabled if both variables are integers greater than zero
```
-e REQPERIOD=2 -e REQCOUNT=15
```
You can set expirations on all messages globally with the EXPIRATION environment variable \
Using EXPIRATION=60 will tell redis to delete each POSTed message 60 seconds after it was added
```
-e EXPIRATION=60
```

## HTTPS Configuration
In order to serve up an https socket, you must provide the key/certificate pair under /cert \
Use the following options to provide the files under /cert
```
-e CERTFILE=cert.pem \
-e KEYFILE=privkey.pem \
-v /srv/docker/mycerts:/cert
```
A full https example with certbot certs:
```
docker run -d --rm --name redis -p 172.17.0.1:6379:6379 redis
docker run -d --name webstore \
-p 51.195.74.100:443:8080 -e REDISIP=172.17.0.1 \
-e CERTFILE=config/live/webstore.mydomain.com/fullchain.pem \
-e KEYFILE=config/live/webstore.mydomain.com/privkey.pem \
-v /srv/docker/certbot:/cert \
fullaxx/webstore
```

## Client Examples using docker
You can test the client docker images against a running server instance \
The test server is hosted at webstore.dspi.org on ports 80 and 443:
```
docker run -it -e WSHOST=webstore.dspi.org -e WSPORT=80 -e ALG=1 -e MSG=test fullaxx/webstore-post
docker run -it -e WSHOST=webstore.dspi.org -e WSPORT=80 -e TOKEN=098f6bcd4621d373cade4e832627b4f6 fullaxx/webstore-get
```
The same example using https:
```
docker run -it -e HTTPS=1 -e WSHOST=webstore.dspi.org -e WSPORT=443 -e ALG=1 -e MSG=test fullaxx/webstore-post
docker run -it -e HTTPS=1 -e WSHOST=webstore.dspi.org -e WSPORT=443 -e TOKEN=098f6bcd4621d373cade4e832627b4f6 fullaxx/webstore-get
```

## Client Instructions
In order to use the client, you will need to compile against libcurl and libgcrypt \
In Ubuntu, you would install build-essential libcurl4-gnutls-dev and libgcrypt20-dev \
```
apt-get install -y build-essential libcurl4-gnutls-dev libgcrypt20-dev
./compile_clients.sh
```
After compiling the client source code, you can run the binaries:
```
WSHOST="172.17.0.1"
WSPORT="80"
./ws_post.exe -H ${WSHOST} -P ${WSPORT} -f LICENSE -a 1
./ws_get.exe  -H ${WSHOST} -P ${WSPORT} -t b234ee4d69f5fce4486a80fdaf4a4263 -f LICENSE.copy
md5sum LICENSE LICENSE.copy
```
If your webstore server is running in https mode:
```
WSHOST="172.17.0.1"
WSPORT="443"
./ws_post.exe -s -H ${WSHOST} -P ${WSPORT} -f LICENSE -a 1
./ws_get.exe  -s -H ${WSHOST} -P ${WSPORT} -t b234ee4d69f5fce4486a80fdaf4a4263 -f LICENSE.copy
md5sum LICENSE LICENSE.copy
```

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
```
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
