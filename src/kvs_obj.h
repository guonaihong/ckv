#ifndef __KVS_OBJ_H
#define __KVS_OBJ_H

#ifdef __cplusplus
extern "C" {
#endif

#define KVS_OBJ_BUF    0
#define KVS_OBJ_LIST   1

typedef struct kvs_obj_t {
    unsigned type;
    void    *ptr;
} kvs_obj_t;

kvs_obj_t *kvs_obj_buf_new(const char *p, size_t len);

kvs_obj_t *kvs_obj_list_new();

void kvs_obj_free(kvs_obj_t *o);
#ifdef __cplusplus
}
#endif

#endif
