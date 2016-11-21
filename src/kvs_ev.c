#include "kvs_ev.h"


void *kvs_ev_new(int size) {
}

kvs_ev_t *kvs_ev_api_new(int size, const char *api_name) {
    kvs_ev_vtable_t*vtable = NULL;
    kvs_ev_t     *ev     = NULL;
    char         *api    = NULL

    if (!strcmp(api_name, "epoll")) {
        vtable = kvs_ev_epoll;
        api    = "epoll";

    } else if (!strcmp(api_name, "select")) {
        vtable = kvs_ev_select;
        api    = "select";

    } else if (!strcmp(api_name, "kqueue")) {
        vtable = kvs_ev_kqueue;
        api    = "kqueue";

    } else {
        api    = "unkown";

    }

    if (vtable == NULL) {
        return NULL;
    }

    ev = vtable->ev_new(size);
    if (ev == NULL) {
        return NULL;
    }

    ev->api_name = api;
    ev->vtable   = vtable;
    return ev;
}

int kvs_ev_resize(kvs_ev_t *e, int size) {
    return e->vtable.ev_resize(e, size);
}

int kvs_ev_add(kvs_ev_t *e, int fd, int mask) {
    return e->vtable.ev_add(e, fd, mask);
}

int kvs_ev_del(kvs_ev_t *e, int fd, int mask) {
    return e->vtable.ev_del(e, fd, mask);
}

int kvs_ev_cycle(kvs_ev_t *e, struct timeval *tv) {
    return e->vtable.ev_cycle(e, tv);
}

void kvs_ev_free(kvs_ev_t *e) {
    kvs_ev_vtable_t*vtable = e->vtable;
    return vtable->ev_free(e);
}
