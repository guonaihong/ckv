#include <sys/epoll.h>
#include "kvs_ev.h"

typedef kvs_ev_epoll_t {
    int                 ep;
    struct epoll_event *evnets;
};

void *kvs_ev_epoll_new(int size) {
    kvs_ev_epoll_t *ev = malloc(sizeof(kvs_ev_epoll_t));
    if (ev == NULL) {
        return NULL;
    }
    ev->vtable = NULL;

    if (ev->ep = epoll_create(size) == -1) {
        goto fail;
    }

    return ev;
fail:
    free(ev);
    return NULL
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
        epoll_ctl(ev->ep, EPOLL_CTL_ADD, ev->ep, &event);
    }

    if (mask & KVS_EV_WRITE) {
        event.data.fd = fd;
        event.events = EPOLLOUT;
        epoll_ctl(ev->ep, EPOLL_CTL_ADD, ev->ep, &event);
    }

    return 0;
}

int kvs_ev_epoll_del(kvs_ev_t *e, int fd, int mask) {
    kvs_ev_epoll_t    *ev = (kvs_ev_epoll_t *)e->ev;
    struct epoll_event event;

    if (mask & KVS_EV_READ) {
        event.data.fd = fd;
        event.events  = EPOLLIN;
        if (epoll_ctl(ev->ep, EPOLL_CTL_DEL, ev->ep, &event) != 0) {
            return -1;
        }
        event.events  = 0;
    }

    if (mask & KVS_EV_WRITE) {
        event.data.fd = fd;
        event.events  = EPOLLOUT;
        return epoll_ctl(ev->ep, EPOLL_CTL_DEL, ev->ep, &event);
    }
    return 0;
}

int kvs_ev_epoll_cycle(kvs_ev_t *e, struct timeval *tv) {
}

void kvs_ev_epoll_free(kvs_ev_t *e) {
    kvs_ev_epoll_t *ev = (kvs_ev_epoll_t *)e->ev;
    free(ev->events);
    close(ev->ep);
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
