#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#include "kvs_ev.h"

typedef struct kvs_ev_epoll_t {
    int                 epfd;
    struct epoll_event *events;
} kvs_ev_epoll_t;

void *kvs_ev_epoll_new(int size) {
    kvs_ev_epoll_t *ev = malloc(sizeof(kvs_ev_epoll_t));
    if (ev == NULL) {
        return NULL;
    }

    if ((ev->epfd = epoll_create(size)) == -1) {
        goto fail;
    }

    if ((ev->events = malloc(sizeof(struct epoll_event) * size)) == NULL) {
        goto fail;
    }

    return ev;
fail:
    free(ev);
    return NULL;
}

int kvs_ev_epoll_resize(kvs_ev_t *e, int size) {
    kvs_ev_epoll_t *ev = (kvs_ev_epoll_t *)e->ev;
    struct epoll_event    *new_events = NULL;

    if ((new_events = realloc(ev->events, size)) == NULL) {
        return -1;
    }
    ev->events = new_events;
    return 0;
}

int debug_ev(int fd, int mask) {
    printf("fd is %d, event(%s)\n", fd, mask & KVS_EV_READ? "read":(mask & KVS_EV_WRITE ? "write": "unknown"));
    return 0;
}

int kvs_ev_epoll_add(kvs_ev_t *e, int fd, int mask) {
    kvs_ev_epoll_t *ev = (kvs_ev_epoll_t *)e->ev;
    struct epoll_event event;

    /* debug */
    int op = (e->cache[fd].mask == KVS_EV_UNUSED) ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    debug_ev(fd, mask);

    mask |= e->cache[fd].mask;
    event.data.fd = fd;
    event.events = 0;
    if (mask & KVS_EV_READ)  event.events |= EPOLLIN;
    if (mask & KVS_EV_WRITE) event.events |= EPOLLOUT;

    if (epoll_ctl(ev->epfd, op, fd, &event) == -1) {
        return -1;
    }

    return 0;
}

int kvs_ev_epoll_del(kvs_ev_t *e, int fd, int mask) {
    kvs_ev_epoll_t    *ev = (kvs_ev_epoll_t *)e->ev;
    struct epoll_event event;
    int op, newmask;
    newmask = e->cache[fd].mask & (~mask);
    op = (newmask == KVS_EV_UNUSED) ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;

    event.events  = 0;
    event.data.fd = fd;
    if (mask & KVS_EV_READ) {
        event.events  |= EPOLLIN;
    }

    if (mask & KVS_EV_WRITE) {
        event.events  |= EPOLLOUT;
    }

    if (epoll_ctl(ev->epfd, op, fd, &event) == -1) {
        return -1;
    }
    return 0;
}

int kvs_ev_epoll_cycle(kvs_ev_t *e, struct timeval *tv) {
    int             i, n;
    kvs_ev_epoll_t *ev = (kvs_ev_epoll_t *)e->ev;

    n = epoll_wait(ev->epfd, ev->events, e->size, tv ? (tv->tv_sec * 1000 + tv->tv_usec / 1000): -1);

    for (i = 0; i < n; i++) {
        e->active[i].fd = ev->events[i].data.fd;
        if (ev->events[i].events & EPOLLIN) {
            e->active[i].mask |= KVS_EV_READ;
        }

        if (ev->events[i].events & EPOLLOUT) {
            e->active[i].mask |= KVS_EV_WRITE;
        }
    }

    return n;
}

void kvs_ev_epoll_free(kvs_ev_t *e) {
    kvs_ev_epoll_t *ev = (kvs_ev_epoll_t *)e->ev;
    free(ev->events);
    close(ev->epfd);
    free(ev);
}

const kvs_ev_vtable_t kvs_ev_epoll = {
    kvs_ev_epoll_new,
    kvs_ev_epoll_add,
    kvs_ev_epoll_del,
    kvs_ev_epoll_resize,
    kvs_ev_epoll_cycle,
    kvs_ev_epoll_free,
};
