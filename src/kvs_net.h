#ifndef __KVS_NET_H
# define __KVS_NET_H

#include "kvs_command.h"
#include "kvs_ev.h"
#include "kvs_str.h"

#ifdef __cplusplus
extern "C" {
#endif
int kvs_net_read(kvs_ev_t *e, int fd, int mask, void *user_data);

kvs_client_t *kvs_client_new(int fd);

int kvs_net_read(kvs_ev_t *e, int fd, int mask, void *user_data);

int kvs_net_write(kvs_ev_t *e, int fd, int mask, void *user_data);

#define KVS_CMD_OK      0x1
#define KVS_CMD_UNUSED  0
#define KVS_PROTO_SIZE 4096
#define KVS_MAX_LINE   64 * 1024

#ifdef __cplusplus
}
#endif

#endif
