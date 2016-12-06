#include <stdio.h>
#include <stdlib.h>
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
    kvs_client_t *c = malloc(sizeof(kvs_client_t));
    if (c == NULL) {
        return NULL;
    }
    c->fd = fd;
    kvs_buf_null(&c->buf);
    return 0;
}

//TODO call command and parse
int kvs_net_parse() {
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
    return 0;
}
