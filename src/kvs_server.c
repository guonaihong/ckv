#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <libgen.h>
#include "kvs_log.h"
#include "kvs_net.h"
#include "kvs_ev.h"
#include "kvs_common.h"
#include "kvs_sock.h"


static int socket_non_blocking(int sfd) {
    int flags, s;

    flags = fcntl (sfd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl(sfd, F_SETFL, flags);
    if (s == -1) {
        return -1;
    }

    return 0;
}

int accept_fd(kvs_ev_t *e, int listen_fd, int mask, void *user_data) {
    int                connfd;
    struct sockaddr_in cli;
    socklen_t          len;
    int                ntry;
    kvs_client_t       *c;

    for (ntry = 3; ntry > 0; ntry--) {
        connfd = accept(listen_fd, (struct sockaddr *)&cli, &len);
        if (connfd == -1) {
            if (errno == EINTR)
                continue;

            if (errno != EWOULDBLOCK || errno != EAGAIN) {
                return -1;
            }
            continue;
        }
        if (socket_non_blocking(connfd) == -1) {
            printf("set socket non-blocking fail:%s:fd(%d)\n", strerror(errno), connfd);
            close(connfd);
            return -1;
        }

        c = kvs_client_new(connfd);
        kvs_ev_add(e, connfd, KVS_EV_READ, kvs_net_read, c);
    }
    return 0;
}

int kvs_server_init(kvs_server_t *s, const char *port, kvs_log_t *log) {

    s->ev        = kvs_ev_api_new(100, "select");
    s->listen_fd = bind_create((char *)port);
    s->log       = log;
    if (s->listen_fd == -1) {
        kvs_log(log, KVS_ERROR, "bind fail:%s\n", strerror(errno));
        return -1;
    }

    if (socket_non_blocking(s->listen_fd) == -1) {
        kvs_log(log, KVS_ERROR, "set listen fd non-blocking fail:%s\n", strerror(errno));
        return -1;
    }

    if (listen(s->listen_fd, 0) == -1) {
        kvs_log(log, KVS_ERROR, "listen fail:%s\n", strerror(errno));
        goto fail;
    }

    s->hash = kvs_hash_new(1024);
    kvs_ev_add(s->ev, s->listen_fd, KVS_EV_READ, accept_fd, NULL);
    return 0;
fail:
    close(s->listen_fd);
    kvs_ev_free(s->ev);
    return -1;
}

int main(int argc, char **argv) {

    kvs_log_t *log;
    log = kvs_log_new(KVS_DEBUG, basename(argv[0]), 0);

    if (kvs_server_init(&kvs_server, "56789", log) == -1) {
        kvs_log(log, KVS_ERROR, "kvs init fail\n");
        kvs_log_free(log);
        return 1;
    }
    kvs_log(log, KVS_DEBUG, "cycle start\n");
    kvs_ev_cycle(kvs_server.ev, NULL);
    kvs_ev_free(kvs_server.ev);
    kvs_log(log, KVS_DEBUG, "bye bye ..\n");
    kvs_log_free(log);
    return 0;
}
