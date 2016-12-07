#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "kvs_net.h"
#include "kvs_common.h"
#define KVS_PROTO_SIZE 4096

int kvs_cmd_set(kvs_server_t *s, kvs_str_t *key, kvs_str_t *val, kvs_str_t *old_val) {
    kvs_hash_add(s->hash, key->p, (size_t)&key->len, (void **)&old_val->p);
    return 0;
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
    kvs_buf_null(&c->buf);
    c->cmd.flags = KVS_CMD_UNUSED;
    return 0;
}

//TODO call command and parse
int kvs_net_parse(kvs_client_t *c) {
    char *p    = NULL;
    int   i    = 0;
    char *last = 0;
    int   nhead= 0;
    int   len;

    p = c->buf.p;

    for (i = 0, len = c->buf.len; i < len; i++) {
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

int kvs_cmd_exec(kvs_client_t *c) {
    if (c->cmd.flags != KVS_CMD_OK)
        return EAGAIN;

    kvs_str_t val;
    if (!strcmp(c->cmd.action.p, "set")) {
        kvs_str_t *v = malloc(sizeof(kvs_str_t));
        memcpy(v, &c->cmd.val, sizeof(kvs_str_t));
        kvs_cmd_set(&kvs_server, &c->cmd.key, v, &val);
    }
    if (!strcmp(c->cmd.action.p, "get")) {
        kvs_cmd_get(&kvs_server, &c->cmd.key, &val);
    }
    if (!strcmp(c->cmd.action.p, "del")) {
        kvs_cmd_del(&kvs_server, &c->cmd.key);
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

        kvs_buf_append(&c->buf, buf, rv);
    }

    kvs_net_parse(c);
    kvs_cmd_exec(c);
    return 0;
}
