#ifndef __KVS_HASH_H
#define __KVS_HASH_H

#ifdef __cplusplus
extern "C" {
#endif

#define KVS_INIT_HASH_SIZE 8
#define KVS_EXPAND         1
#define KVS_UNUSED        -1
    
typedef struct kvs_hash_node_t {
    const void             *key;
    int                     klen;
    void                   *val;
    struct kvs_hash_node_t *next;
} kvs_hash_node_t;

typedef struct kvs_hash_bucket_t {
    kvs_hash_node_t **buckets;
    int               use;
    int               size;
    int               mask;
} kvs_hash_bucket_t;

typedef struct kvs_hash_t {
    kvs_hash_bucket_t *buckets;
    kvs_hash_bucket_t *expand_buckets;
    int index;
    unsigned int (*hash_func)(const unsigned char *s, int *klen);
} kvs_hash_t;

kvs_hash_t *kvs_hash_new(int hint);
int kvs_hash_del(kvs_hash_t *h, const void *k, size_t klen, void **v);
int kvs_hash_add(kvs_hash_t *h, const void *k, size_t klen, void **v);
kvs_hash_node_t *kvs_hash_find(kvs_hash_t *h, const void *k, size_t klen);

#ifdef __cplusplus
}
#endif
#endif
