#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "kvs_hash.h"
#include "kvs_command.h"
#include "kvs_common.h"
#include "kvs_net.h"
#include "kvs_obj.h"
#include "kvs_log.h"

#define MSG_OK    "+OK\r\n"
#define MSG_ERR   "$-1\r\n"
#define MSG_TRUE  ":1\r\n"
#define MSG_FALSE ":0\r\n"

int kvs_cmd_set(kvs_server_t *s, kvs_buf_t *key, kvs_obj_t *val) {
    int rv;

    kvs_log(s->log, KVS_DEBUG, "%s\n", __func__);
    kvs_obj_t **old_val = &val;

    rv = kvs_hash_add(s->hash, key->p, (size_t)key->len, (void **)old_val);
    if (rv == 1) {
        kvs_obj_free(*old_val);
    }

    return  rv;
}

int kvs_cmd_get(kvs_server_t *s, kvs_buf_t *key, kvs_buf_t **val) {
    kvs_log(s->log, KVS_DEBUG, "%s\n", __func__);

    kvs_obj_t *p;

    p = kvs_hash_find(s->hash, key->p, key->len);
    if (p == NULL) {
        kvs_log(s->log, KVS_DEBUG, "not found:\n");
        return -1;
    }

    if (p->type != KVS_OBJ_BUF) {
        return -1;
    }
    *val = p->ptr;

    /* debug */
    char *str = strndup((*val)->p, (*val)->len + 1);
    kvs_log(s->log, KVS_DEBUG, "get result:%s\n", str);
    free(str);
    return 0;
}

int kvs_cmd_del(kvs_server_t *s, kvs_buf_t *key) {
    kvs_log(s->log, KVS_DEBUG, "del\n");

    void *v;
    int   rv;
    rv = kvs_hash_del(s->hash, key->p, key->len, &v);
    if (rv == -1) {
        return -1;
    }
    kvs_obj_free(v);
    return 0;
}

void kvs_command_set(kvs_client_t *c) {

    /* write ok */
    kvs_cmd_set(&kvs_server, c->argv[1]->ptr, c->argv[2]);
    c->argv[2] = NULL;
    /* TODO memory too small write fail */

    memcpy(c->wbuf, MSG_OK, sizeof(MSG_OK) -1);
    c->wpos = sizeof(MSG_OK) - 1;
    /* write -1 */
}

void kvs_command_get(kvs_client_t *c) {

    kvs_buf_t *val = NULL;
    int        n   = 0;

    if (kvs_cmd_get(&kvs_server, c->argv[1]->ptr, &val) == -1) {
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

    if (c->argc != 2) {
        /* TODO write error */
    }

    if (kvs_cmd_del(&kvs_server, c->argv[1]->ptr) == 0) {
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
    kvs_log(kvs_server.log, KVS_DEBUG, "%s\n", __func__);
}

void kvs_command_rpush(kvs_client_t *c) {
    kvs_log(kvs_server.log, KVS_DEBUG, "%s\n", __func__);
}

void kvs_command_lange(kvs_client_t *c) {
    kvs_log(kvs_server.log, KVS_DEBUG, "%s\n", __func__);
}

void kvs_command_llen(kvs_client_t *c) {
    kvs_log(kvs_server.log, KVS_DEBUG, "%s\n", __func__);
}

void kvs_command_lpop(kvs_client_t *c) {
    kvs_log(kvs_server.log, KVS_DEBUG, "%s\n", __func__);
}

void kvs_command_rpop(kvs_client_t *c) {
    printf("%s\n", __func__);
    kvs_log(kvs_server.log, KVS_DEBUG, "%s\n", __func__);
}

void kvs_command_lterm(kvs_client_t *c) {
    kvs_log(kvs_server.log, KVS_DEBUG, "%s\n", __func__);
}

void kvs_command_init() {
    static const kvs_command_t cmds[] = {
        {"set",   kvs_command_set,   3},
        {"get",   kvs_command_get,   2},
        {"del",   kvs_command_del,   2},
        {"lpush", kvs_command_lpush, 3},
        {"rpush", kvs_command_rpush, 3},
        {"lange", kvs_command_lange, 3},
        {"llen",  kvs_command_llen,  2},
        {"lpop",  kvs_command_lpop,  2},
        {"rpop",  kvs_command_rpop,  2},
        {"lterm", kvs_command_lterm, 2},
        {NULL, NULL, 0}
    };

    kvs_command_t  *c, *tmp;
    kvs_server.cmds = kvs_hash_new(sizeof(cmds) / sizeof(cmds[0]));

    for (c = (kvs_command_t *)cmds; c->name; c++) {
        tmp = c;
        kvs_hash_add(kvs_server.cmds, c->name, strlen(c->name), (void **)&tmp);
    }
}

int kvs_command_process(kvs_client_t *c) {
    kvs_log(kvs_server.log, KVS_DEBUG, "%s:flag = %d\n", __func__, c->flags);

    if (!(c->flags & KVS_CMD_OK))
        return EAGAIN;

    kvs_command_t *cmd;
    int            rv;


    kvs_str_t *argv0 = c->argv[0]->ptr;

    cmd = kvs_hash_find(kvs_server.cmds, argv0->p, argv0->len);

    if (!cmd) {
        /* todo remove event and write error message */
        kvs_log(kvs_server.log, KVS_WARN, "unknown command:%s\n", argv0->p);
    } else {
        if (cmd->nargs < c->argc) {
            /* todo write error */
            kvs_log(kvs_server.log, KVS_WARN, "The number of parameters is incorrect\n");
        }
        cmd->cmd(c);
    }

    kvs_log(kvs_server.log, KVS_DEBUG, "write check:    argv flag (%d) #buffer wpos = %d\n", c->flags, c->wpos);
    if ((c->flags & KVS_CMD_OK) && c->wpos != 0) {
        rv = kvs_ev_add(kvs_server.ev, c->fd, KVS_EV_WRITE, kvs_net_write, c);
        kvs_log(kvs_server.log, KVS_DEBUG, "call write rv = %d\n", rv);
    }
    return 0;
}

void kvs_command_free() {
    kvs_hash_free(kvs_server.cmds, NULL);
}
