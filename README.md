# webstore
A web-based arbitrary data storage service that accepts z85 encoded data

## Base Docker Image
[Ubuntu](https://hub.docker.com/_/ubuntu) 20.04 (x64)

## Software
[libmicrohttpd](https://www.gnu.org/software/libmicrohttpd/) \
[libgcrypt](https://gnupg.org/software/libgcrypt/index.html) \
[redis](https://redis.io/)

## Get the image from Docker Hub
```
docker pull fullaxx/webstore
```

## Volume Options
webstore will create a log file in /log/ \
Use the following volume option to expose it
```
-v /srv/docker/webstore/log:/log
```

## Run the image
Start redis at 172.17.0.1:6379 \
Bind webstore to 172.17.0.1:8080 \
Save log to /srv/docker/webstore/log
```
docker run -d --rm --name redis -p 172.17.0.1:6379:6379 redis
docker run -d --name webstore -e REDISIP=172.17.0.1 -p 172.17.0.1:8080:8080 -v /srv/docker/webstore/log:/log fullaxx/webstore
```
Start redis at 172.17.0.1:7777 \
Bind webstore to 172.17.0.1:8080
```
docker run -d --rm --name redis -p 172.17.0.1:7777:6379 redis
docker run -d --name webstore -e REDISIP=172.17.0.1 -e REDISPORT=7777 -p 172.17.0.1:8080:8080 fullaxx/webstore
```

## Build it locally using the github repository
```
docker build -f Dockerfile.webstore -t="fullaxx/webstore" github.com/Fullaxx/webstore
```
