#!/bin/zsh

str=$(ls ../resources | grep ".gif" | tr "\n" " ")
arr=(${(@s: :)str})
str=$(ls ../resources)
idx=$(shuf -i 1-${#arr[@]} -n 1)

#echo "idx = "${idx} #> errfile
arr[${idx}]="../resources/${arr[${idx}]}"
#echo "file = ${arr[${idx}]}"
echo -n "HTTP/1.1 200 OK\r\n"
echo -n "Content-Type: "$(file -b --mime-type ${arr[${idx}]})"\r\n"
echo -n "Content-Length: "$(stat --printf="%s" ${arr[${idx}]})"\r\n\r\n"
#To fix: if cat command below is commented out, will affect the other cgi script that follows this one
cat ${arr[${idx}]}
