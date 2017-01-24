#ifndef __KVS_LOG_H
#define __KVS_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
typedef struct kvs_log_t kvs_log_t;

struct kvs_log_t {
    int              level;
    char            *prog;
    int              flags;
    pthread_mutex_t  lock;
};

#define KVS_LOG_ATOMIC 0x1
#define KVS_DEBUG      0x2
#define KVS_INFO       0x4
#define KVS_WARN       0x8
#define KVS_ERROR      0x10

#define BUF_SIZE 1024
kvs_log_t *kvs_log_new(int level, char *prog, int flags);

int kvs_log(kvs_log_t *log, int level, const char *fmt, ...);

void kvs_log_free(kvs_log_t *log);
#ifdef __cplusplus
}
#endif

#endif
