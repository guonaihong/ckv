#ifndef __KVS_SOCK_H
#define __KVS_SOCK_H

#if __cplusplus
extern "C" {
#endif

int bind_create(char *port);

int conn_create(char *host, char *port);
#if __cplusplus
}
#endif

#endif
