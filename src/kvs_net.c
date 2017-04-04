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
    c->fd = fd;
    kvs_buf_null(&c->rbuf);
    c->argv.flags = KVS_CMD_UNUSED;
    c->wpos      = 0;
    return c;
}

void kvs_client_free(kvs_client_t *c) {
    kvs_buf_free(&c->rbuf);
}

void kvs_cmd_reset(kvs_cmd_t *argv) {
    kvs_str_null(&argv->action);
    kvs_str_null(&argv->key);
    kvs_str_null(&argv->val);
    argv->flags = KVS_CMD_UNUSED;
}

void kvs_client_reset(kvs_client_t *c) {

    c->pos  = 0;
    c->wpos = 0;
    kvs_cmd_reset(&c->argv);
}

int kvs_net_unmarshal(kvs_client_t *c) {
    char *p    = NULL;
    int   i    = 0;
    char *last = 0;
    int   nhead= 0;
    int   len;

    p = c->rbuf.p;

    kvs_log(kvs_server.log, KVS_DEBUG, "%s:buffer(l:%d, a:%d, p:%d, wp:%d)\n",
            __func__, c->rbuf.len, c->rbuf.alloc, c->pos, c->wpos);

    for (i = 0, len = c->rbuf.len; i < len; ) {
        if(p[i] == '*') {
            i++;
            c->nargs= strtoul(p + i, &last, 10);
            if (errno == ERANGE) {
                //TODO
            }
            i = last - p;
            if (*last == '\r') i++;
            if (last[1] == '\n') i++;

            c->pos = i;
        }

        //TODO: Wrong protocol format, exit parsing
        if (p[i] == '$') {
            i++;
            nhead = strtoul(p + i, &last, 10);
            if (errno == ERANGE) {
                //TODO
            }

            i = last - p;
            if (*last == '\r') i++;
            if (last[1] == '\n') i++;

            if (c->argv.action.len == 0) {

                kvs_log(kvs_server.log, KVS_DEBUG, "nhead = %d\n", nhead);
                c->argv.action.p   = p + i;
                c->argv.action.len = nhead;

                kvs_log(kvs_server.log, KVS_DEBUG, "set argv\n");
            } else if (c->argv.key.len == 0) {
                c->argv.key.p   = p + i;
                c->argv.key.len = nhead;

                kvs_log(kvs_server.log, KVS_DEBUG, "set key\n");
            } else if (c->argv.val.len == 0) {
                c->argv.val.p   = p + i;
                c->argv.val.len = nhead;
                c->argv.flags |= KVS_CMD_OK;
                kvs_log(kvs_server.log, KVS_DEBUG, "%d:### set val\n", c->argv.flags);

            }
            if (i + nhead < len) {
                i += nhead;
            }

            if (p[i] == '\r') i++;
            if (p[i] == '\n') i++;

            if (c->argv.flags & KVS_CMD_OK) {
                kvs_buf_truncate(&c->rbuf, i);
                return 0;
            }
        }

    }

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

    kvs_net_unmarshal(c);
    kvs_command_process(c);

    kvs_log(kvs_server.log, KVS_DEBUG, "read rv = %d\n", rv);
    if (rv == 0) {
        kvs_ev_del(kvs_server.ev, c->fd, KVS_EV_READ|KVS_EV_WRITE, kvs_net_write, c);
        kvs_client_free(c);
        close(fd);
    }
    return 0;
}
