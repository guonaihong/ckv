#ifndef __KVS_COMMAND_H
# define __KVS_COMMAND_H

#include "kvs_common.h"
#ifdef __cplusplus
extern "C" {
#endif
void kvs_command_init();
int  kvs_command_process(kvs_client_t *c);
void kvs_command_free();
#ifdef __cplusplus
}
#endif

#endif
