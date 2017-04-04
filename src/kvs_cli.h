#ifndef __KVS_CLI_H
#define __KVS_CLI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "kvs_str.h"
typedef struct kvs_cli_t kvs_cli_t;

struct kvs_cli_t {
    int         fd;
    const char *port;
    kvs_str_t  *argv;
    int         argc;
};

#ifdef __cplusplus
extern "C" }
#endif
#endif
