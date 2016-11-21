#ifndef __KVS_EV_H
#define __KVS_EV_H

#ifdef __cplusplus
extern "C" {
#endif

#define KVS_EV_READ  0x01
#define KVS_EV_WRITE 0x02

typedef struct kvs_ev_vtable_tkvs_ev_vtable_t;

typedef struct kvs_ev_t {
    int           maxfd;
    void         *ev;
    int           size;
    char         *api;
    kvs_ev_vtable_t*vtable;
} kvs_ev_t;

typedef struct kvs_ev_vtable_t{
    void *(*ev_new)(int size);
    int   (*ev_add)(kvs_ev_t *e, int fd, int mask);
    int   (*ev_del)(kvs_ev_t *e, int fd, int mask);
    int   (*ev_resize)(kvs_ev_t *e, int size);
    int   (*ev_cycle)(kvs_ev_t *e, struct timeval *tv);
    void  (*ev_free)(kvs_ev_t *e);
} kvs_ev_vtable_t;

kvs_ev_t *kvs_ev_api_new(int size, const char *api_name);

int kvs_ev_resize(kvs_ev_t *e, int size);

int kvs_ev_add(kvs_ev_t *e, int fd, int mask);

int kvs_ev_del(kvs_ev_t *e, int fd, int mask);

int kvs_ev_cycle(kvs_ev_t *e, struct timeval *tv);

void kvs_ev_free(kvs_ev_t *e);

#ifdef __cplusplus
}
#endif

#endif
