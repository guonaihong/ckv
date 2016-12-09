#ifndef __KVS_NET_H
#define __KVS_NET_H

#include "kvs_ev.h"
#include "kvs_str.h"
#ifdef __cplusplus
extern "C" {
#endif
int kvs_net_read(kvs_ev_t *e, int fd, int mask, void *user_data);

typedef struct kvs_cmd_t kvs_cmd_t;
struct kvs_cmd_t {
    kvs_str_t action;
    kvs_str_t key;
    kvs_str_t val;
    int       flags;
};

#define KVS_OUTPUT_SIZE 16 * 1024
typedef struct kvs_client_t kvs_client_t;
struct kvs_client_t {
    kvs_buf_t rbuf;
    int       fd;
    int       pos;
    int       nhead;
    char      wbuf[KVS_OUTPUT_SIZE]; //small content values
    int       wpos;
    kvs_cmd_t cmd;
};

kvs_client_t *kvs_client_new(int fd);

#define KVS_CMD_OK      0
#define KVS_CMD_UNUSED -1
#ifdef __cplusplus
}
#endif

#endif
