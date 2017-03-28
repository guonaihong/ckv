#ifndef __KVS_OBJ_H
#define __KVS_OBJ_H

#ifdef __cplusplus
extern "C" {
#endif

#define KVS_OBJ_STR    0
#define KVS_OBJ_LIST   1

struct kvs_obj_t {
    unsigned type;
    void    *ptr;
};

#ifdef __cplusplus
}
#endif

#endif
