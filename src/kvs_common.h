#ifndef __KVS_COMMON_H
#define __KVS_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif
#include "kvs_hash.h"
#include "kvs_str.h"
#include "kvs_log.h"

typedef struct kvs_server_t kvs_server_t;
struct kvs_server_t {
    kvs_ev_t   *ev;
    kvs_hash_t *hash;
    kvs_log_t  *log;
    int         listen_fd;
};

kvs_server_t kvs_server;

#ifdef __cplusplus
}
#endif
#endif
