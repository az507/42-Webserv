#!/bin/zsh

#argv[1]/$1 for this script is the directory we want to list out
echo -n "200 HTTP/1.1 OK\r\nContent-type: text/html\r\n\r\n"
cd $1
docker run -v $PWD:/home az507/directory-list:v1 -H . -L 1
