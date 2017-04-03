#include <stdlib.h>
#include "kvs_obj.h"
#include "kvs_list.h"
#include "kvs_str.h"

kvs_obj_t *kvs_obj_new(int type, void *ptr) {
    kvs_obj_t *o = malloc(sizeof(kvs_obj_t));
    if (o == NULL) {
        return NULL;
    }
    o->type = type;
    o->ptr  = ptr;
    return o;
}

kvs_obj_t *kvs_obj_str_new(const char *p, size_t len) {

    kvs_buf_t *s = kvs_buf_new(p, len);
    return kvs_obj_new(KVS_OBJ_BUF, s);
}

kvs_obj_t *kvs_obj_list_new() {

    kvs_list_t *list = kvs_list_new();
    return kvs_obj_new(KVS_OBJ_LIST, list);
}

void kvs_obj_buf_free(kvs_obj_t *o) {
    kvs_buf_free((kvs_buf_t *)o->ptr);
    free(o);
}

void kvs_obj_list_free(kvs_obj_t *o) {
    kvs_list_free((kvs_list_t *)o->ptr);
    free(o);
}
