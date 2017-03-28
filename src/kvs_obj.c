#include "kvs_obj.h"

kvs_obj_t *kvs_obj_new(int type, void *ptr) {
}

kvs_obj_t *kvs_obj_str_new(const char *p, size_t len) {

    kvs_str_t *s = kvs_str_new(p, len);
    return kvs_obj_new(KVS_OBJ_STR, s);
}

#if 0
kvs_obj_t *kvs_obj_list_new() {

    kvs_list_t *list = kvs_list_new();
    return kvs_obj_new(KVS_OBJ_LIST, list);
}
#endif
