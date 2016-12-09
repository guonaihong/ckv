#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "kvs_net.h"
#include "kvs_common.h"
#define KVS_PROTO_SIZE 4096

int kvs_cmd_set(kvs_server_t *s, kvs_str_t *key, kvs_str_t *val, kvs_str_t *old_val) {
    return kvs_hash_add(s->hash, key->p, (size_t)&key->len, (void **)&old_val->p);
}

int kvs_cmd_get(kvs_server_t *s, kvs_str_t *key, kvs_str_t *val) {
    kvs_hash_node_t *p;

    p = kvs_hash_find(s->hash, key->p, key->len);
    if (p == NULL) {
        return -1;
    }

    *val = *(kvs_str_t *)p->val;
    return 0;
}

int kvs_cmd_del(kvs_server_t *s, kvs_str_t *key) {
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
    return 0;
}

int kvs_net_unmarshal(kvs_client_t *c) {
    char *p    = NULL;
    int   i    = 0;
    char *last = 0;
    int   nhead= 0;
    int   len;

    p = c->rbuf.p;

    for (i = 0, len = c->rbuf.len; i < len; i++) {
        if(p[i] == '*') {
            i++;
            c->nhead = strtoul(p + i, &last, 10);
            if (errno == ERANGE) {
            }
            if (*last == '\r') i++;
            if (*last == '\n') i++;

            c->pos = i;
        }

        if (*p == '$') {
            i++;
            nhead = strtoul(p + i, &last, 10);
            if (errno == ERANGE) {
                //TODO
            }
            if (c->cmd.action.len == 0) {
                c->cmd.key.p   = p + i;
                c->cmd.key.len = nhead;
            } else if (c->cmd.key.len == 0) {
                c->cmd.key.p   = p + i;
                c->cmd.key.len = nhead;
            } else if (c->cmd.val.len == 0) {
                c->cmd.val.p   = p + i;
                c->cmd.val.len = nhead;
            }
            if (i + nhead < len) {
                i += nhead;
            }
            if (*last == '\r') i++;
            if (*last == '\n') i++;
        }

    }
    return 0;
}

int kvs_net_write(kvs_ev_t *e, int fd, int mask, void *user_data) {

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
    if (c->cmd.flags != KVS_CMD_OK)
        return EAGAIN;

    kvs_str_t val;
    int       n;

    kvs_str_null(&val);

    if (!strcmp(c->cmd.action.p, "set")) {
        kvs_str_t *v = malloc(sizeof(kvs_str_t));
        memcpy(v, &c->cmd.val, sizeof(kvs_str_t));

        /* write ok */
        if (kvs_cmd_set(&kvs_server, &c->cmd.key, v, &val) != -1) {
            free(val.p);
            return 0;
        }

        memcpy(c->wbuf, MSG_OK, sizeof(MSG_OK) -1);
        c->wpos = sizeof(MSG_OK) - 1;
        /* write -1 */
    }

    if (!strcmp(c->cmd.action.p, "get")) {
        /* write value */
        if (kvs_cmd_get(&kvs_server, &c->cmd.key, &val) == -1) {
            /* write -1 */
            c->wpos = snprintf(c->wbuf, KVS_PROTO_SIZE, MSG_ERR);
            return -1;
        }

        if (val.len < KVS_OUTPUT_SIZE - 32 + 3) {
            n = snprintf(c->wbuf, KVS_PROTO_SIZE, "$%d\r\n", val.len);
            memcpy(c->wbuf + n, val.p, val.len);
            c->wpos = n + val.len;
        }

    }

    if (!strcmp(c->cmd.action.p, "del")) {
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

    if (c->cmd.flags == KVS_CMD_OK && c->wpos != 0) {
        kvs_ev_add(kvs_server.ev, c->fd, KVS_EV_WRITE, kvs_net_write, c);
    }
    return 0;
}

int kvs_net_read(kvs_ev_t *e, int fd, int mask, void *user_data) {
    char buf[KVS_PROTO_SIZE];
    int  rv;

    kvs_client_t *c = (kvs_client_t *)user_data;
    for (;;) {
        rv = read(fd, buf, KVS_PROTO_SIZE);
        if (rv == -1) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        kvs_buf_append(&c->rbuf, buf, rv);
    }

    kvs_net_unmarshal(c);
    kvs_cmd_exec(c);
    return 0;
}
