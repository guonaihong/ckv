#ifndef __KVS_HASH_H
#define __KVS_HASH_H

#ifdef __cplusplus
extern "C" {
#endif

#define KVS_INIT_HASH_SIZE 8
    
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
    kvs_hash_bucket_t *oldbuckets;

    unsigned int (*hash_func)(const unsigned char *s, int *klen);
    unsigned expand;
} kvs_hash_t;

kvs_hash_t *kvs_hash_new(int hint);

#ifdef __cplusplus
}
#endif
#endif
