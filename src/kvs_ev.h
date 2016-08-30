#ifndef __KVS_EV_H
#define __KVS_EV_H

#ifdef __cplusplus
extern "C" {
#endif

typedef kvs_ev_t {
    int   maxfd;
    void *ev;
    int   size;
} kvs_ev_t;

#ifdef __cplusplus
}
#endif

#endif
