ARG FPATH=$FILEPATH
FROM debian:11
RUN apt update && apt install tree
WORKDIR /
ADD $FPATH /
ENTRYPOINT [ "tree" ]
