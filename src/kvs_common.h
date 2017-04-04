#ifndef __KVS_COMMON_H
#define __KVS_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif
#include "kvs_hash.h"
#include "kvs_str.h"
#include "kvs_ev.h"
#include "kvs_log.h"

typedef struct kvs_server_t kvs_server_t;
struct kvs_server_t {
    kvs_ev_t   *ev;
    kvs_hash_t *hash;
    kvs_log_t  *log;
    kvs_hash_t *cmds;
    int         listen_fd;
};

typedef struct kvs_command_t kvs_command_t;
typedef struct kvs_cmd_t kvs_cmd_t;
typedef struct kvs_client_t kvs_client_t;

struct kvs_command_t {
    char *name;
    void (*cmd)(kvs_client_t *c);
};

struct kvs_cmd_t {
    kvs_str_t action;
    kvs_str_t key;
    kvs_str_t val;
    unsigned  flags;
};

#define KVS_OUTPUT_SIZE 16 * 1024
struct kvs_client_t {
    kvs_buf_t      rbuf;
    int            fd;
    int            pos;
    int            nargs;
    char           wbuf[KVS_OUTPUT_SIZE]; //small content values
    int            wpos;
    kvs_cmd_t      argv;
    kvs_command_t *command;
};

kvs_server_t kvs_server;

#ifdef __cplusplus
}
#endif
#endif
