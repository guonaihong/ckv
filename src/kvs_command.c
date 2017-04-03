#include <stdlib.h>
#include <string.h>
#include "kvs_hash.h"
#include "kvs_command.h"
#include "kvs_common.h"
#include "kvs_net.h"

#define MSG_OK    "+OK\r\n"
#define MSG_ERR   "$-1\r\n"
#define MSG_TRUE  ":1\r\n"
#define MSG_FALSE ":0\r\n"

int kvs_cmd_set(kvs_server_t *s, kvs_str_t *key, kvs_str_t *val, kvs_str_t **old_val) {
    int rv;

    kvs_log(s->log, KVS_DEBUG, "%s\n", __func__);
    *old_val = val;
    kvs_log(s->log, KVS_DEBUG, "1111 val = %p, old_val = %p \n", val, *old_val);
    rv = kvs_hash_add(s->hash, key->p, (size_t)key->len, (void **)old_val);
    kvs_log(s->log, KVS_DEBUG, "2222 val = %p, old_val = %p rv = (%d)\n", val, *old_val, rv);
    return  rv;
}

int kvs_cmd_get(kvs_server_t *s, kvs_str_t *key, kvs_str_t **val) {
    kvs_log(s->log, KVS_DEBUG, "%s\n", __func__);
    kvs_hash_node_t *p;

    p = kvs_hash_find(s->hash, key->p, key->len);
    if (p == NULL) {
        kvs_log(s->log, KVS_DEBUG, "not found:\n");
        return -1;
    }

    *val = p->val;

    char *str = strndup((*val)->p, (*val)->len + 1);
    kvs_log(s->log, KVS_DEBUG, "get result:%s\n", str);
    free(str);
    return 0;
}

int kvs_cmd_del(kvs_server_t *s, kvs_str_t *key) {
    kvs_log(s->log, KVS_DEBUG, "get");

    void *v;
    int   rv;
    rv = kvs_hash_del(s->hash, key->p, key->len, &v);
    if (rv == -1) {
        return -1;
    }
    free(v);
    return 0;
}

void kvs_command_set(kvs_client_t *c) {
    int         rv;
    kvs_str_t  *val;
    kvs_str_t  *v    = kvs_str_new(c->cmd.val.p, c->cmd.val.len);

    /* write ok */
    if ((rv = kvs_cmd_set(&kvs_server, &c->cmd.key, v, &val) == 1)) {
        /* TODO memory too small write fail */
        kvs_str_free(val);
    }

    memcpy(c->wbuf, MSG_OK, sizeof(MSG_OK) -1);
    c->wpos = sizeof(MSG_OK) - 1;
    /* write -1 */
}

void kvs_command_get(kvs_client_t *c) {

    kvs_str_t *val = NULL;
    int        n   = 0;

    if (kvs_cmd_get(&kvs_server, &c->cmd.key, &val) == -1) {
        /* write -1 */
        c->wpos = snprintf(c->wbuf, KVS_PROTO_SIZE, MSG_ERR);
        return ;
    }

    if (val->len < KVS_OUTPUT_SIZE - 32 + 3) {
        n = snprintf(c->wbuf, KVS_PROTO_SIZE, "$%d\r\n", val->len);
        memcpy(c->wbuf + n, val->p, val->len);
        c->wpos = n + val->len;
    }
}

void kvs_command_del(kvs_client_t *c) {
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

void kvs_command_lpush(kvs_client_t *c) {

}

void kvs_command_rpush(kvs_client_t *c) {

}

void kvs_command_lange(kvs_client_t *c) {

}

void kvs_command_llen(kvs_client_t *c) {
}

void kvs_command_lpop(kvs_client_t *c) {
}

void kvs_command_rpop(kvs_client_t *c) {

}

void kvs_command_lterm(kvs_client_t *c) {

}

void kvs_command_init() {
    kvs_command_t cmds[] = {
        {"set", kvs_command_set},
        {"get", kvs_command_get},
        {"get", kvs_command_del},
        {"lpush", kvs_command_lpush},
        {"rpush", kvs_command_rpush},
        {"lange", kvs_command_lange},
        {"llen", kvs_command_llen},
        {"lpop", kvs_command_lpop},
        {"rpop", kvs_command_rpop},
        {"lterm", kvs_command_lterm},
    };
}
