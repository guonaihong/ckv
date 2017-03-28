#ifdef __KVS_TIME_MINHEAP_H
#define __KVS_TIME_MINHEAP_H

#ifdef __cplusplus
extern "C" {
#endif

#define KVS_MIN_SIZE          8
#define KVS_MINHEAP_LEFT(i)   2 *(i) + 1
#define KVS_MINHEAP_RIGHT(i)  2 * (i) + 2
#define KVS_MINHEAP_PARENT(i) ((i) - 1) /2

typedef struct kvs_minheap_t {
    void        **arr;
    int           pos;
    int           nalloc;
    int           (*cmp)(void *v, void *v1);
    void          (myfree)(void *);

    unsigned char flag;
} kvs_minheap_t;

kvs_minheap_t *kvs_minheap_new();

int kvs_minheap_init(kvs_minheap_t *minheap);

int kvs_minheap_add(kvs_minheap_t *minheap, void *t);

int kvs_minheap_del(kvs_minheap_t *minheap, void **t);

void kvs_minheap_free(kvs_minheap_t *minheap);
#ifdef __cplusplus
}
#endif

#endif
