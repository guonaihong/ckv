#include <stdio.h>

typedef kvs_ev_select_t {
    fd_set r, w;
    fd_set read, write;
    int    maxfd;
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

int kvs_ev_select_resize(int size) {
    if (size > sizeof(fd_set))
        return -1;
}

int kvs_ev_select_add(void *e, int fd, int mask) {
    kvs_ev_select_t *ev = (kvs_ev_select_t *)e;

    if (mask & KVS_EV_READ) {
        FD_SET(fd, &ev->r);
    }

    if (mask & KVS_EV_WRITE) {
        FD_SET(fd, &ev->w);
    }

    return 0;
}

int kvs_ev_select_del(void *e, int fd, int mask) {
    kvs_ev_select_t *ev = (kvs_ev_select_t *)e;

    if (mask & KVS_EV_READ) {
        FD_CLR(fd, &ev->r);
    }

    if (mask & KVS_EV_WRITE) {
        FD_CLR(fd, &ev->w);
    }

    return 0;
}

int kvs_ev_select_cycle(void *e, struct timeval *tv) {
    int i, rv;
    memcpy(&e->read, &e->r, sizeof(e->r));
    memcpy(&e->write, &e->w, sizeof(e->write));

    rv = select(e->maxfd, &e->read, &e->write, NULL, tv);
    for (i = 0; i < rv; i++) {
    }
}

void kvs_ev_select_free(void *e) {
    free(e);
}
