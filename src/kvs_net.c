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
    c->cmd.flags = KVS_CMD_UNUSED;
    c->wpos      = 0;
    return c;
}

void kvs_client_free(kvs_client_t *c) {
    kvs_buf_free(&c->rbuf);
}

void kvs_cmd_reset(kvs_cmd_t *cmd) {
    kvs_str_null(&cmd->action);
    kvs_str_null(&cmd->key);
    kvs_str_null(&cmd->val);
    cmd->flags = KVS_CMD_UNUSED;
}

void kvs_client_reset(kvs_client_t *c) {

    c->pos  = 0;
    c->wpos = 0;
    kvs_cmd_reset(&c->cmd);
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

            if (c->cmd.action.len == 0) {

                kvs_log(kvs_server.log, KVS_DEBUG, "nhead = %d\n", nhead);
                c->cmd.action.p   = p + i;
                c->cmd.action.len = nhead;

                kvs_log(kvs_server.log, KVS_DEBUG, "set cmd\n");
            } else if (c->cmd.key.len == 0) {
                c->cmd.key.p   = p + i;
                c->cmd.key.len = nhead;

                kvs_log(kvs_server.log, KVS_DEBUG, "set key\n");
            } else if (c->cmd.val.len == 0) {
                c->cmd.val.p   = p + i;
                c->cmd.val.len = nhead;
                c->cmd.flags |= KVS_CMD_OK;
                kvs_log(kvs_server.log, KVS_DEBUG, "%d:### set val\n", c->cmd.flags);

            }
            if (i + nhead < len) {
                i += nhead;
            }

            if (p[i] == '\r') i++;
            if (p[i] == '\n') i++;

            if (c->cmd.flags & KVS_CMD_OK) {
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

int kvs_cmd_exec(kvs_client_t *c) {
    kvs_log(kvs_server.log, KVS_DEBUG, "%s:flag = %d\n", __func__, c->cmd.flags);
    if (!(c->cmd.flags & KVS_CMD_OK))
        return EAGAIN;

    kvs_str_t *val;
    int        rv;

    val = NULL;

    //if (!strncmp(c->cmd.action.p, "set", sizeof("set") - 1))

    kvs_log(kvs_server.log, KVS_DEBUG, "write check:    cmd flag (%d) #buffer wpos = %d\n", c->cmd.flags, c->wpos);
    if ((c->cmd.flags & KVS_CMD_OK) && c->wpos != 0) {
        rv = kvs_ev_add(kvs_server.ev, c->fd, KVS_EV_WRITE, kvs_net_write, c);
        kvs_log(kvs_server.log, KVS_DEBUG, "call write rv = %d\n", rv);
    }
    return 0;
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
    kvs_cmd_exec(c);

    kvs_log(kvs_server.log, KVS_DEBUG, "read rv = %d\n", rv);
    if (rv == 0) {
        kvs_ev_del(kvs_server.ev, c->fd, KVS_EV_READ|KVS_EV_WRITE, kvs_net_write, c);
        kvs_client_free(c);
        close(fd);
    }
    return 0;
}
