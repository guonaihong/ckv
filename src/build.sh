#!/bin/bash
gcc kvs_server.c kvs_ev.c kvs_ev_epoll.c kvs_ev_select.c kvs_str.c kvs_hash.c kvs_net.c -Wall -okvs_server -g
