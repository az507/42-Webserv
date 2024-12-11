#!/bin/zsh

echo $# | cat -e
echo $1 | cat -e
echo $0 | cat -e
echo -n "HTTP/1.1 200 OK\r\n"
echo -n "Content-type: text/plain\r\n";
echo -n "Connection: keep-alive\r\n\r\n";
# $1 is for mounting the current directory to the docker image
docker build -t autoindex --build-arg $1 
# $1 is for argument for tree cmd in docker container
docker run autoindex $1
