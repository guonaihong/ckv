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
#include "kvs_net.h"
#include "kvs_ev.h"
#include "kvs_common.h"

int bind_create(char *port) {
    struct addrinfo  hints;
    struct addrinfo *result, *rp;
    int s, sfd;

    memset (&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;     /* All interfaces */

    s = getaddrinfo(NULL, port, &hints, &result);
    if (s != 0) {
        fprintf (stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;

        s = bind(sfd, rp->ai_addr, rp->ai_addrlen);
        if (s == 0) {
            /* We managed to bind successfully! */
            break;
        }

        close(sfd);
    }

    if (rp == NULL) {
        fprintf (stderr, "Could not bind\n");
        return -1;
    }

    freeaddrinfo(result);

    return sfd;
}

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
        }
        if (socket_non_blocking(connfd) == -1) {
            printf("set socket non-blocking fail:%s\n", strerror(errno));
            close(connfd);
            return -1;
        }

        c = kvs_client_new(connfd);
        kvs_ev_add(e, connfd, KVS_EV_READ, kvs_net_read, c);
    }
    return 0;
}

int kvs_server_init(kvs_server_t *s, const char *port) {

    s->ev = kvs_ev_api_new(100, "epoll");
    s->listen_fd = bind_create((char *)port);
    if (s->listen_fd == -1) {
        printf("bind fail:\n");
        return -1;
    }

    if (socket_non_blocking(s->listen_fd) == -1) {
        printf("set listen fd non-blocking fail:%s\n", strerror(errno));
        return -1;
    }

    kvs_ev_add(s->ev, s->listen_fd, KVS_EV_READ, accept_fd, NULL);
    return 0;
}

int main() {

    if (kvs_server_init(&kvs_server, "56789") == -1) {
        printf("kvs init fail\n");
        return 1;
    }
    kvs_ev_cycle(kvs_server.ev, NULL);
    kvs_ev_free(kvs_server.ev);
    printf("bye bye ..\n");
    return 0;
}
