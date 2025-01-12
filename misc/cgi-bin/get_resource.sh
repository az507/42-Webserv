#!/bin/zsh

str=$(ls ../resources | tr "\n" " ")
arr=(${(@s: :)str})
str=$(ls ../resources)
idx=$(shuf -i 2-${#arr[@]} -n 1)

arr[${idx}]="../resources/${arr[${idx}]}"
echo "HTTP/1.1 200 OK"
echo "Content-Type: "$(file -b --mime-type ${arr[${idx}]})
echo "Content-Length: "$(stat --printf="%s" ${arr[${idx}]})
echo ""
#To fix: if cat command below is commented out, will affect the other cgi script that follows this one
cat ${arr[${idx}]}
