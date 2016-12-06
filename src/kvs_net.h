#ifndef __KVS_NET_H
#define __KVS_NET_H

#include "kvs_ev.h"
#include "kvs_str.h"
#ifdef __cplusplus
extern "C" {
#endif
int kvs_net_read(kvs_ev_t *e, int fd, int mask, void *user_data);

typedef struct kvs_client_t kvs_client_t;
struct kvs_client_t {
    int       fd;
    kvs_buf_t buf;
};

kvs_client_t *kvs_client_new(int fd);

#ifdef __cplusplus
}
#endif

#endif
