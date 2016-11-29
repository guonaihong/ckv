#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
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

int kvs_ev_epoll_add(kvs_ev_t *e, int fd, int mask) {
    kvs_ev_epoll_t *ev = (kvs_ev_epoll_t *)e->ev;
    struct epoll_event event;

    if (mask & KVS_EV_READ) {
        event.data.fd = fd;
        event.events  = EPOLLIN;
        epoll_ctl(ev->epfd, EPOLL_CTL_ADD, ev->epfd, &event);
    }

    if (mask & KVS_EV_WRITE) {
        event.data.fd = fd;
        event.events = EPOLLOUT;
        epoll_ctl(ev->epfd, EPOLL_CTL_ADD, ev->epfd, &event);
    }

    return 0;
}

int kvs_ev_epoll_del(kvs_ev_t *e, int fd, int mask) {
    kvs_ev_epoll_t    *ev = (kvs_ev_epoll_t *)e->ev;
    struct epoll_event event;

    if (mask & KVS_EV_READ) {
        event.data.fd = fd;
        event.events  = EPOLLIN;
        if (epoll_ctl(ev->epfd, EPOLL_CTL_DEL, ev->epfd, &event) != 0) {
            return -1;
        }
        event.events  = 0;
    }

    if (mask & KVS_EV_WRITE) {
        event.data.fd = fd;
        event.events  = EPOLLOUT;
        return epoll_ctl(ev->epfd, EPOLL_CTL_DEL, ev->epfd, &event);
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
