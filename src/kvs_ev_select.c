#include <stdio.h>
#include "kvs_ev.h"

typedef kvs_ev_select_t {
    fd_set r, w;
    fd_set read, write;
} kvs_ev_select_t;

void *kvs_ev_select_new(int size) {
    (void)size;

    ev = (kvs_ev_select_t *)sizeof(kvs_ev_select_t);
    if (s == NULL)  {
        return NULL;
    }

    FD_ZERO(&ev->r);
    FD_ZERO(&ev->w);
    return ev;
}

int kvs_ev_select_resize(kvs_ev_t *e, int size) {

    if (size > sizeof(e->r))
        return -1;
}

int kvs_ev_select_add(kvs_ev_t *e, int fd, int mask) {
    kvs_ev_select_t *ev = (kvs_ev_select_t *)e->ev;

    if (mask & KVS_EV_READ) {
        FD_SET(fd, &ev->r);
    }

    if (mask & KVS_EV_WRITE) {
        FD_SET(fd, &ev->w);
    }

    return 0;
}

int kvs_ev_select_del(kvs_ev_t *e, int fd, int mask) {
    kvs_ev_select_t *ev = (kvs_ev_select_t *)e->ev;

    if (mask & KVS_EV_READ) {
        FD_CLR(fd, &ev->r);
    }

    if (mask & KVS_EV_WRITE) {
        FD_CLR(fd, &ev->w);
    }

    return 0;
}

int kvs_ev_select_cycle(kvs_ev_t *e, struct timeval *tv) {
    int i, rv;
    kvs_ev_select_t *ev = (kvs_ev_select_t *)e->ev;

    memcpy(&ev->read, &ev->r, sizeof(ev->r));
    memcpy(&ev->write, &ev->w, sizeof(ev->write));

    rv = select(e->maxfd, &ev->read, &ev->write, NULL, tv);
    for (i = 0; i < rv; i++) {
    }
}

void kvs_ev_select_free(kvs_ev_t *e) {
    free(e->ev);
}
