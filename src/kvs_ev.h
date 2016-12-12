#ifndef __KVS_EV_H
#define __KVS_EV_H

#include <sys/select.h>
#ifdef __cplusplus
extern "C" {
#endif

#define KVS_EV_READ    0x01
#define KVS_EV_WRITE   0x02
#define KVS_EV_UNUSED  0x0
#define KVS_EV_ERR     -1

typedef struct kvs_ev_vtable_t kvs_ev_vtable_t;
typedef struct kvs_ev_cache_t kvs_ev_cache_t;
typedef struct kvs_ev_active_t kvs_ev_active_t;
typedef struct kvs_ev_t        kvs_ev_t;
typedef int (*kvs_ev_proc_t)(kvs_ev_t *e, int fd, int mask, void *user_data);

struct kvs_ev_cache_t{
    int   mask;
    kvs_ev_proc_t read_proc;
    kvs_ev_proc_t write_proc;
    void *user_data;
};

struct kvs_ev_active_t{
    int fd;
    int mask;
};

struct kvs_ev_t {
    void         *ev;
    char         *api_name;
    int           size;
    int           maxfd;

    kvs_ev_cache_t *cache;
    kvs_ev_active_t *active;
    kvs_ev_vtable_t *vtable;
    unsigned char stop;
};

struct kvs_ev_vtable_t{
    void *(*ev_new)(int size);
    int   (*ev_add)(kvs_ev_t *e, int fd, int mask);
    int   (*ev_del)(kvs_ev_t *e, int fd, int mask);
    int   (*ev_resize)(kvs_ev_t *e, int size);
    int   (*ev_cycle)(kvs_ev_t *e, struct timeval *tv);
    void  (*ev_free)(kvs_ev_t *e);
};

kvs_ev_t *kvs_ev_api_new(int size, const char *api_name);
int kvs_ev_add(kvs_ev_t *e, int fd, int mask, kvs_ev_proc_t proc, void *user_data);
int kvs_ev_del(kvs_ev_t *e, int fd, int mask, kvs_ev_proc_t porc, void *user_data);
int kvs_ev_resize(kvs_ev_t *e, int new_size);
void kvs_ev_stop(kvs_ev_t *e);
int kvs_ev_cycle(kvs_ev_t *e, struct timeval *tv);
void kvs_ev_free(kvs_ev_t *e);

#ifdef __cplusplus
}
#endif

#endif
