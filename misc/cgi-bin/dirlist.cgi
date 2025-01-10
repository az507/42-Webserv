#!/bin/zsh

#argv[1]/$1 for this script is the directory we want to list out
echo -n "200 HTTP/1.1 OK\r\nContent-type: text/html\r\n\r\n"
#echo $1 | cat -e
#pwd | cat -e
#str=$1
#if [ $1 -eq "~" ]; then
#    str=$PWD
#fi
#
#echo $str
#exit 1

if  test -d $1; then
    cd $1
    pwd | cat -e
    docker run -v $PWD:/home az507/directory-list:v1 -H . -L 1
else
    cat $1
fi
