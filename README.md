# Webstore
A web-based arbitrary data storage/retrieval service built from
* [libmicrohttpd](https://www.gnu.org/software/libmicrohttpd/) 
* [libgcrypt](https://gnupg.org/software/libgcrypt/index.html) 
* [redis](https://redis.io/)

Webstore consists of 3 components (server, GET client, POST client) \
which you can compile yourself or use pre-built images docker. \
For manual compilation instructions see [COMPILE.md](https://github.com/Fullaxx/webstore/blob/master/COMPILE.md)

# Using Webstore Docker Containers
The easiest way to work with the webstore components is by using docker. \
You can pull the base docker image and immediately begin using the project.

## Base Docker Image
[Ubuntu](https://hub.docker.com/_/ubuntu) 20.04 (x64)

## Get the image from Docker Hub
```
docker pull fullaxx/webstore
docker pull fullaxx/webstore-get
docker pull fullaxx/webstore-post
```

## Build it locally using the github repository
```
docker build -f Dockerfile.webstore -t="fullaxx/webstore"      github.com/Fullaxx/webstore
docker build -f Dockerfile.wsget    -t="fullaxx/webstore-get"  github.com/Fullaxx/webstore
docker build -f Dockerfile.wspost   -t="fullaxx/webstore-post" github.com/Fullaxx/webstore
```

## Run the Containers

### Server
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

### Clients for GET and POST
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

## Advanced Server Configuration Options
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
Using EXPIRATION=60 will tell redis to delete each message 60 seconds after it was POSTed
```
-e EXPIRATION=60
```
You can set a flag that will make all messages immutable \
Using IMMUTABLE=1 will allow a successful POST only if that hash doesnt already exist in redis \
If IMMUTABLE=1 is set any incoming POST with a hash that already exists in redis will be returned with 304
```
-e IMMUTABLE=1
```
You can set a flag that will allow only 1 GET per message \
Using BAR=1 will tell redis to delete the retrieved message after a successful GET
```
-e BAR=1
```
By default the web server code will run in a single thread \
Set MULTITHREAD=1 to enable a new thread for each incoming connection
```
-e MULTITHREAD=1
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
