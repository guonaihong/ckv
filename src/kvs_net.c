#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "kvs_net.h"
#include "kvs_common.h"
#define KVS_PROTO_SIZE 4096

int kvs_cmd_set(kvs_server_t *s, kvs_str_t *key, kvs_str_t *val, kvs_str_t **old_val) {
    printf("%s\n", __func__);
    *old_val = val;
    printf("1111 val = %p, old_val = %p \n", val, *old_val);
    int rv;
    rv = kvs_hash_add(s->hash, key->p, (size_t)key->len, (void **)old_val);
    printf("2222 val = %p, old_val = %p \n", val, *old_val);
    return  rv;
}

int kvs_cmd_get(kvs_server_t *s, kvs_str_t *key, kvs_str_t **val) {
    printf("%s\n", __func__);
    kvs_hash_node_t *p;

    p = kvs_hash_find(s->hash, key->p, key->len);
    if (p == NULL) {
        printf("no found:\n");
        return -1;
    }

    *val = p->val;

    char *str = strndup((*val)->p, (*val)->len + 1);
    printf("get result:%s\n", str);
    return 0;
}

int kvs_cmd_del(kvs_server_t *s, kvs_str_t *key) {
    printf("get\n");
    void *v;
    int   rv;
    rv = kvs_hash_del(s->hash, key->p, key->len, &v);
    if (rv == -1) {
        return -1;
    }
    free(v);
    return 0;
}

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

int kvs_net_unmarshal(kvs_client_t *c) {
    char *p    = NULL;
    int   i    = 0;
    char *last = 0;
    int   nhead= 0;
    int   len;

    p = c->rbuf.p;

    printf("%s\n", __func__);
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

                printf("nhead = %d\n", nhead);
                c->cmd.action.p   = p + i;
                c->cmd.action.len = nhead;

                printf("set cmd\n");
            } else if (c->cmd.key.len == 0) {
                c->cmd.key.p   = p + i;
                c->cmd.key.len = nhead;

                printf("set key\n");
            } else if (c->cmd.val.len == 0) {
                c->cmd.val.p   = p + i;
                c->cmd.val.len = nhead;
                c->cmd.flags |= KVS_CMD_OK;
                printf("%d:### set val\n", c->cmd.flags);
            }
            if (i + nhead < len) {
                i += nhead;
            }

            if (p[i] == '\r') i++;
            if (p[i] == '\n') i++;

            if (c->cmd.flags & KVS_CMD_OK) {
                return 0;
            }
        }

    }

    return 0;
}

int kvs_net_write(kvs_ev_t *e, int fd, int mask, void *user_data) {

    printf(":::%s\n", __func__);
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
    }
    return cn;
}

#define MSG_OK    "+OK\r\n"
#define MSG_ERR   "$-1\r\n"
#define MSG_TRUE  ":1\r\n"
#define MSG_FALSE ":0\r\n"

int kvs_cmd_exec(kvs_client_t *c) {
    printf("%s\n", __func__);
    if (!(c->cmd.flags & KVS_CMD_OK))
        return EAGAIN;

    kvs_str_t *val;
    int       n, rv;

    val = NULL;

    if (!strncmp(c->cmd.action.p, "set", sizeof("set") - 1)) {
        kvs_str_t *v = kvs_str_new(c->cmd.val.p, c->cmd.val.len);
        /* write ok */
        if ((rv = kvs_cmd_set(&kvs_server, &c->cmd.key, v, &val) == 1)) {
            /* TODO memory too small write fail */
            kvs_str_free(val);
            return 0;
        }

        memcpy(c->wbuf, MSG_OK, sizeof(MSG_OK) -1);
        c->wpos = sizeof(MSG_OK) - 1;
        /* write -1 */
    }

    if (!strncmp(c->cmd.action.p, "get", sizeof("get") - 1)) {
        /* write value */
        if (kvs_cmd_get(&kvs_server, &c->cmd.key, &val) == -1) {
            /* write -1 */
            c->wpos = snprintf(c->wbuf, KVS_PROTO_SIZE, MSG_ERR);
            return -1;
        }

        if (val->len < KVS_OUTPUT_SIZE - 32 + 3) {
            n = snprintf(c->wbuf, KVS_PROTO_SIZE, "$%d\r\n", val->len);
            memcpy(c->wbuf + n, val->p, val->len);
            c->wpos = n + val->len;
        }

    }

#if 0
    if (!strncmp(c->cmd.action.p, "del", sizeof("del") - 1)) {
        if (kvs_cmd_del(&kvs_server, &c->cmd.key) == 0) {
            /* write 1 */
            memcpy(c->wbuf, MSG_TRUE, sizeof(MSG_TRUE) - 1);
            c->wpos = sizeof(MSG_TRUE) -1;
        } else {
            memcpy(c->wbuf, MSG_FALSE, sizeof(MSG_FALSE) - 1);
            c->wpos = sizeof(MSG_FALSE) -1;
        /* write 0 */
        }
    }

#endif

    printf("%d:::wpos = %d\n", c->cmd.flags, c->wpos);
    if ((c->cmd.flags & KVS_CMD_OK) && c->wpos != 0) {
        rv = kvs_ev_add(kvs_server.ev, c->fd, KVS_EV_WRITE, kvs_net_write, c);
        printf("call write rv = %d\n", rv);
    }
    return 0;
}

int kvs_net_read(kvs_ev_t *e, int fd, int mask, void *user_data) {
    char buf[KVS_PROTO_SIZE];
    int  rv;

    printf("start %s\n", __func__);
    kvs_client_t *c = (kvs_client_t *)user_data;
    for (;;) {
        rv = read(fd, buf, KVS_PROTO_SIZE - 1);
        /* printf("rv = %d errno(%s):fd(%d)\n", rv, strerror(errno), fd); */
        if (rv == -1) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (rv == 0) {
            break;
        }

        kvs_buf_append(&c->rbuf, buf, rv);
        printf("read buf is %s:len(%d)\n", c->rbuf.p, c->rbuf.len);
    }

    kvs_net_unmarshal(c);
    kvs_cmd_exec(c);

    if (rv == 0) {
        close(fd);
    }
    return 0;
}
