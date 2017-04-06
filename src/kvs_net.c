#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "kvs_net.h"
#include "kvs_common.h"

kvs_client_t *kvs_client_new(int fd) {

    kvs_client_t *c = calloc(1, sizeof(kvs_client_t));

    if (c == NULL) {
        return NULL;
    }

    kvs_buf_null(&c->rbuf);
    c->fd    = fd;
    c->argv  = NULL; //TODO free
    c->argc  = 0;
    c->nhead = 0;
    c->flags = KVS_CMD_UNUSED;
    c->wpos  = 0;

    return c;
}

void kvs_client_free(kvs_client_t *c) {
    kvs_buf_free(&c->rbuf);
}

void kvs_client_reset(kvs_client_t *c) {

    c->wpos = 0;
    c->flags = KVS_CMD_UNUSED;
}

int kvs_net_parse(kvs_client_t *c) {
    char *p     = NULL;
    char *last  = NULL;
    int   nhead = 0, pos = 0;

    p   = c->rbuf.p;

    kvs_log(kvs_server.log, KVS_DEBUG, "%s:buffer(l:%d, a:%d, wp:%d)\n",
            __func__, c->rbuf.len, c->rbuf.alloc, c->wpos);

    if (c->nargs == 0) {
        /* check */
        last = strchr(p, '\r');
        if (last == NULL) {
            if (last - p > KVS_MAX_LINE) {
                /* todo write error */
            }
            return -1;
        }

        if (last[1] != '\n') {
            return -1;
        }
        last++;

        if (p[pos] != '*') {
            return -1;
        }

        /* TODO check nargc size */
        nhead = strtoul(p + pos + 1, NULL, 10);
        c->nargs = nhead;
        pos = 1 + last - p;

        if (c->argv) {
            free(c->argv);
        }
        c->argv = malloc(sizeof(kvs_obj_t *) * c->nargs);

        kvs_log(kvs_server.log, KVS_DEBUG, "nargs = %d:(%s)\n", c->nargs, p+pos);
    }

    while (c->nargs > 0) {

        if (c->nhead == 0) {
            last = strchr(p+pos, '\r');
            if (last == NULL) {
                if (last - p > KVS_MAX_LINE) {
                    /* todo write error */
                }
                return -1;
            }

            if (last[1] != '\n') {
                return -1;
            }
            last++;

            if (p[pos] != '$') {
                return -1;
            }

            nhead = strtoul(p + pos + 1, NULL, 10);
            c->nhead = nhead;
            pos = 1 + last - p;
        }

        c->argv[c->argc++] = kvs_obj_buf_new(p + pos, nhead);

        /*debug*/
        kvs_buf_t *buf = c->argv[c->argc - 1]->ptr;
        kvs_log(kvs_server.log, KVS_DEBUG, "(%s)\n", buf->p);

        pos += c->nhead + 2;
        c->nhead = 0;
        c->nargs--;
    }

    c->flags = KVS_CMD_OK;
    return 0;
}

int kvs_net_write(kvs_ev_t *e, int fd, int mask, void *user_data) {

    kvs_log(kvs_server.log, KVS_DEBUG, ":::%s\n", __func__);
    kvs_client_t *c;
    int           rv, cn;

    c  = (kvs_client_t *)user_data;

    for (cn = 0; cn < c->wpos;) {

        rv = write(c->fd, c->wbuf + cn, c->wpos - cn);

        if (rv == -1) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        cn += rv;
    }

    if (c->wpos - cn == 0) {
        kvs_ev_del(kvs_server.ev, c->fd, KVS_EV_WRITE, kvs_net_write, c);
        kvs_log(kvs_server.log, KVS_DEBUG, "reset client:%p\n", c);
        kvs_client_reset(c);
    }
    return cn;
}

int kvs_net_read(kvs_ev_t *e, int fd, int mask, void *user_data) {
    char buf[KVS_PROTO_SIZE];
    int  rv, cnt;

    kvs_log(kvs_server.log, KVS_DEBUG, "start %s\n", __func__);
    kvs_client_t *c = (kvs_client_t *)user_data;

    cnt = 0;
    for (;;) {
        rv = read(fd, buf, KVS_PROTO_SIZE - 1);

        kvs_log(kvs_server.log, KVS_DEBUG, "* %s:rv = %d errno(%s):fd(%d)\n",
                __func__, rv, strerror(errno), fd);

        if (rv == -1) {
            if (errno == EINTR) {
                continue;
            }

            if (cnt == 0 && errno == EAGAIN) {
                return 0;
            }

            break;
        }

        if (rv == 0) {
            break;
        }

        cnt += rv;
        kvs_buf_append(&c->rbuf, buf, rv);
        kvs_log(kvs_server.log, KVS_DEBUG, "read buf is %s:len(%d)\n", c->rbuf.p, c->rbuf.len);
    }

    if (c->rbuf.len > 0) {
        kvs_net_parse(c);
    }

    if (c->argc > 0) {
        kvs_command_process(c);
        c->argc = 0;
        kvs_buf_reset(&c->rbuf);
    }

    kvs_log(kvs_server.log, KVS_DEBUG, "read rv = %d\n", rv);
    if (rv == 0) {
        kvs_ev_del(kvs_server.ev, c->fd, KVS_EV_READ|KVS_EV_WRITE, kvs_net_write, c);
        kvs_client_free(c);
        close(fd);
    }
    return 0;
}
