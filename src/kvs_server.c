#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include "kvs_ev.h"

int main() {

    int listen;

    struct sockaddr_in sin;

    sin.sin_family      = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port        = htons(1324);

    kvs_ev_t *ev = kvs_ev_api_new(100, "epoll");

    listen = socket(AF_INET, SOCK_STREAM, 0);
    // accept

    return 0;
}
