#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kvs_ev.h"

static kvs_ev_t *kvs_ev_api_select(int size, const char *api_name);
void kvs_ev_free_core(kvs_ev_t *e);

kvs_ev_t *kvs_ev_api_new(int size, const char *api_name) {
    int       i;
    kvs_ev_t *ev = kvs_ev_api_select(size, api_name);

    if (ev == NULL) {
        return NULL;
    }

    ev->cache = malloc(sizeof(kvs_ev_cache_t) * size);
    ev->active = malloc(sizeof(kvs_ev_active_t) * size);

    if (ev->cache == NULL|| ev->active == NULL) {
        goto err;
    }

    for (i = 0; i < size; i++) {
        ev->active[i].mask = KVS_EV_UNUSED;
    }

    return ev;
err:
    if (ev) {
        free(ev->cache);
        free(ev->active);
        kvs_ev_free_core(ev);
    }
    return NULL;
}

kvs_ev_t *kvs_ev_api_select(int size, const char *api_name) {
    kvs_ev_vtable_t *vtable = NULL;
    kvs_ev_t        *ev     = NULL;
    char            *api    = NULL;

    if (!strcmp(api_name, "epoll")) {
#ifdef KVS_EV_EPOLL
        extern kvs_ev_vtable_t kvs_ev_epoll;
        vtable = &kvs_ev_epoll;
        api    = "epoll";

#endif
    } else if (!strcmp(api_name, "select")) {
        extern kvs_ev_vtable_t kvs_ev_select;
        vtable = &kvs_ev_select;
        api    = "select";

    } else if (!strcmp(api_name, "kqueue")) {
#ifdef KVS_EV_KQUEUE
        extern kvs_ev_vtable_t kvs_ev_kqueue;
        vtable = &kvs_ev_kqueue;
        api    = "kqueue";
#endif

    } else {
        api    = "unkown";

    }

    if (vtable == NULL) {
        return NULL;
    }

    ev = malloc(sizeof(kvs_ev_t));
    if (ev == NULL) {
        return NULL;
    }

    ev->ev = vtable->ev_new(size);
    if (ev == NULL) {
        return NULL;
    }

    ev->api_name = api;
    ev->vtable   = vtable;
    ev->size     = size;
    return ev;
}

int kvs_ev_resize_core(kvs_ev_t *e, int size) {
    return e->vtable->ev_resize(e, size);
}

int kvs_ev_add_core(kvs_ev_t *e, int fd, int mask) {
    return e->vtable->ev_add(e, fd, mask);
}

int kvs_ev_del_core(kvs_ev_t *e, int fd, int mask) {
    return e->vtable->ev_del(e, fd, mask);
}

int kvs_ev_cycle_core(kvs_ev_t *e, struct timeval *tv) {
    return e->vtable->ev_cycle(e, tv);
}

void kvs_ev_free_core(kvs_ev_t *e) {
    kvs_ev_vtable_t*vtable = e->vtable;
    return vtable->ev_free(e);
}

int kvs_ev_add(kvs_ev_t *e, int fd, int mask, kvs_ev_proc_t proc, void *user_data) {
    if (fd >= e->size) {
        return KVS_EV_ERR;
    }

    if (mask == KVS_EV_UNUSED) {
        return 0;
    }

    if (kvs_ev_add_core(e, fd, mask) == -1) {
        return KVS_EV_ERR;
    }

    if (mask & KVS_EV_READ) {
        e->cache[fd].read_proc = proc;
    }

    if (mask & KVS_EV_WRITE) {
        e->cache[fd].write_proc = proc;
    }

    e->cache[fd].user_data = user_data;
    e->cache[fd].mask |= mask;

    if (fd > e->maxfd) {
        e->maxfd = fd;
    }

    return 0;
}

int kvs_ev_del(kvs_ev_t *e, int fd, int mask, kvs_ev_proc_t porc, void *user_data) {
    kvs_ev_cache_t *cache;
    int              i;

    if (fd >= e->size) {
        return KVS_EV_ERR;
    }

    if (mask == KVS_EV_UNUSED) {
        return 0;
    }

    if (kvs_ev_del_core(e, fd, mask) == -1) {
        return -1;
    }

    cache = &e->cache[fd];
    cache->mask = cache->mask & (~mask);

    if (e->size == fd) {
        for (i = e->size; i > 0; i--) {
            if (e->cache[i].mask == KVS_EV_UNUSED) {
                continue;
            }
            break;
        }
        e->maxfd = i;
    }
    return 0;
}

int kvs_ev_resize(kvs_ev_t *e, int new_size) {
    int   i;
    void *newptr;

    if (e->size == new_size) {
        return 0;
    }

    if (e->maxfd >= new_size) {
        return KVS_EV_ERR;
    }

    if (kvs_ev_resize_core(e, new_size) == -1) {
        return KVS_EV_ERR;
    }

    newptr = realloc(e->cache, sizeof(kvs_ev_cache_t) * new_size);
    if (newptr == NULL) {
        return KVS_EV_ERR;
    }
    e->cache = newptr;

    newptr = realloc(e->active, sizeof(kvs_ev_active_t) * new_size);
    if (newptr == NULL) {
        return KVS_EV_ERR;
    }
    e->active = newptr;

    for (i = e->size; i < new_size; i++) {
        e->cache[i].mask = KVS_EV_UNUSED;
    }

    e->size = new_size;

    return 0;
}

void kvs_ev_stop(kvs_ev_t *e) {
    e->stop = 1;
}

int kvs_ev_cycle(kvs_ev_t *e, struct timeval *tv) {
    int              i, n, fd, mask;
    kvs_ev_cache_t *cache;

    for (;;) {
        n = kvs_ev_cycle_core(e, tv);
        for(i = 0; i < n; i++) {
            cache = &e->cache[e->active[i].fd];
            mask   = e->active[i].mask;
            fd     = e->active[i].fd;

            if (cache->mask & mask & KVS_EV_READ) {
                cache->read_proc(e, fd, mask, cache->user_data);
            }

            if (cache->mask & mask & KVS_EV_WRITE) {
                cache->write_proc(e, fd, mask, cache->user_data);
            }
        }
    }

    return 0;
}

void kvs_ev_free(kvs_ev_t *e) {
    free(e->cache);
    free(e->active);
    kvs_ev_free_core(e);
    free(e);
}
