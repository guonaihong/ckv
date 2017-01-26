#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "kvs_cli.h"
#include "kvs_str.h"
#include "kvs_sock.h"
#include "kvs_net.h"

/* "*3\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$7\r\nmyvalue\r\n" */
int marshal(kvs_cli_t *c, kvs_cmd_t *cmd) {

    // 支持snprintf 和write buf

    kvs_buffer_t buf;

    kvs_buffer_null(&buf);

    kvs_buf_append_sprintf(&buf, "*3\r\n$%d\r\n", cmd->action.len);

    kvs_buf_append((kvs_buf_t *)&buf, cmd->action.p, cmd->action.len);

    kvs_buf_append_sprintf(&buf, "\r\n$%d\r\n", cmd->key.len);

    kvs_buf_append((kvs_buf_t *)&buf, cmd->key.p, cmd->key.len);

    kvs_buf_append_sprintf(&buf, "\r\n$%d\r\n", cmd->val.len);

    kvs_buf_append((kvs_buf_t *)&buf, cmd->val.p, cmd->val.len);

    kvs_buf_append((kvs_buf_t *)&buf, "\r\n", 2);

    return write(c->fd, buf.p, buf.len);
}

kvs_cli_t *kvs_cli_new(const char *host, const char *port) {
    kvs_cli_t *c = malloc(sizeof(kvs_cli_t));
    if (c == NULL) {
        return NULL;
    }

    c->port = NULL;

    c->fd = conn_create((char *)host, (char *)port);
    if (c->fd == -1) {
        goto cleanup;
    }

    c->port = strdup(port);
    if (c->port == NULL) {
        goto cleanup1;
    }
    return c;

cleanup1:
    close(c->fd);
cleanup:
    free(c);
    return NULL;
}

int kvs_cli_send(kvs_cli_t *c, kvs_cmd_t *cmd) {
    return marshal(c, cmd);
}

int kvs_cli_recv(kvs_cli_t *c, char *rsp, int rsp_len) {
    return read(c->fd, rsp, rsp_len);
}

int main(int argc, char **argv) {
    char line[1024];
    int  rv;

    kvs_cli_t *c = kvs_cli_new("0", "56789");
    if (c == NULL) {
        return 1;
    }

    char title[128];
    int  title_len;

    title_len = snprintf(title, sizeof(title) - 1, "127.0.0.1:%s> ", c->port);

    char key[513] = "";
    char val[513] = "";

    kvs_cmd_t cmd;
    for (;;) {
        write(1, title, title_len);
        rv = read(0, line, sizeof(line) -1);

        if (!strncmp("quit", line, 4)) {
            break;
        }

        if (!strncmp(line, "set", 3)||
            !strncmp(line, "get", 3)||
            !strncmp(line, "del", 3)) {

            cmd.action.p   = line;
            cmd.action.len = 3;
            //TODO
            sscanf(line + 3, "%512s%512s", key, val);
            //printf("key(%s) val(%s)\n", key, val);
            cmd.key.p = key;
            cmd.key.len = strlen(key);

            cmd.val.p = val;
            cmd.val.len = strlen(val);

            if (kvs_cli_send(c, &cmd) == -1) {
                printf("send data fail\n");
            }

            rv = kvs_cli_recv(c, val, sizeof(val));

            char *p    = NULL;
            char *last = NULL;
            if (rv > 1) {
                if (val[0] == '$') {
                    rv = strtoul(val + 1, &p, 10);

                    if (*p == '\r') p++;

                    if (*p == '\n') p++;

                } else {
                    p = val + 1;

                    last = strchr(p, '\r');
                    if (last != NULL) *last = '\0';

                    rv -= 1;
                }

                if (rv == -1) {
                    strcpy(p, "nil");
                } else {
                    p[rv] = '\0';
                }
                printf("\"%s\"\n", p);
            }
            val[0] = '\0';
        }

    }

    return 0;
}
