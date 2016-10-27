#include "kvs_ev.h"
#include "kvs_select.h"

static int kvs_select_new(kvs_ev_t *ev);
static int kvs_select_add(kvs_ev_t *ev, int fd, int mask);
static int kvs_select_del(kvs_ev_t *ev, int fd, int mask);
static int kvs_select_cycle(kvs_ev_t *ev, int fdsz);
static int kvs_select_free(kvs_ev_t *ev);

kvs_ev_conf_t select_conf = {
    kvs_select_new,
    kvs_select_add,
    kvs_select_del,
    kvs_select_cycle,
    kvs_select_free,
};

typedef struct kvs_ev_select {
    fd_set r, w;
    fd_set read, write;
    int    maxfd;
} kvs_ev_select_t;

static int kvs_select_new(kvs_ev_t *ev) {
    kvs_ev_select_t *ev = NULL;

    if (ev == NULL) {
        return -1;
    }
    
    ev = malloc(sizeof(*ev));
    if (ev == NULL) {
        return -1;
    }

    FD_ZERO(&ev->r);
    FD_ZERO(&ev->w);
    return 0;
}

static int kvs_select_add(kvs_ev_t *ev, int fd, int mask) {
    kvs_ev_select_t *ev_select;

    ev_select = (kvs_ev_select_t *)ev->ev_data;

    if (mask & KVS_EV_READ) {
        FD_SET(fd, ev_select->r);
    }

    if (mask & KVS_EV_WRITE) {
        FD_SET(fd, ev_select->w);
    }
    return 0;
}

static int kvs_select_del(kvs_ev_t *ev, int fd, int mask) {
    kvs_ev_select_t *ev_select;

    ev_select = (kvs_ev_select_t *)ev->ev_data;

    if (mask & KVS_EV_READ) {
        FD_CLR(fd, ev_select->r);
    }

    if (mask & KVS_EV_WRITE) {
        FD_CLR(fd, ev_select->w);
    }
    return 0;
}

static int kvs_select_cycle(kvs_ev_t *ev, struct timeval *tv) {
    kvs_ev_select_t *ev_select;
    int              rv, i;

    ev_select = (kvs_ev_select_t *)ev->ev_data;
    rv        = 0;

    memcpy(ev_select->read, ev_select->r, sizeof(fd_set));
    memcpy(ev_select->write, ev_select->w, sizeof(fd_set));

    rv = select(ev_select->maxfd, ev_select->read, ev_select->write, NULL, tv);
    if (rv > 0) {
        for (i = 0; i < rv ;i++) {
        }
    }
    return rv;
}

static void kvs_select_free(kvs_ev_t *ev) {
#if 0
    kvs_ev_select_t *ev_select;

    ev_select = (kvs_ev_select_t *)ev->ev_data;
#endif
    free(ev->ev_data);
}
