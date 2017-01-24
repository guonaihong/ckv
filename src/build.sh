#!/bin/bash

OS=`uname`

if [[ $OS == "Darwin" ]];then
    gcc kvs_server.c kvs_ev.c kvs_ev_select.c kvs_str.c kvs_hash.c kvs_net.c kvs_log.c kvs_sock.c -Wall -okvs_server -g -lm
else
    gcc kvs_server.c kvs_ev.c kvs_ev_epoll.c kvs_ev_select.c kvs_str.c kvs_hash.c kvs_net.c kvs_log.c kvs_sock.c -Wall -okvs_server -g -lm
fi

gcc kvs_cli.c kvs_sock.c kvs_str.c -Wall -o kvs_cli -g
