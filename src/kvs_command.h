#ifndef __KVS_COMMAND_H
# define __KVS_COMMAND_H

#include "kvs_net.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct kvs_command_t {
    char *name;
    void (*cmd)(kvs_client_t *c);
} kvs_command_t;

#ifdef __cplusplus
}
#endif

#endif
