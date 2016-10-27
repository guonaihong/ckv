#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "kvs_hash.h"

#define KVS_KEY_STR -1
unsigned int default_hash(const unsigned char *k, int *klen) {
    int                  i;
    const unsigned char *p;
    unsigned int         hash;

    hash = 0;
    if (*klen == KVS_KEY_STR) {
        for (p = (unsigned char *)k; *p; p++) {
            hash = hash * 33 + *p;
        }
        *klen = p - k;
    } else {
        for (p = k, i = *klen; i > 0; i--, p++) {
            hash = hash * 33 + *p;
        }
    }
    return hash;
}

kvs_hash_bucket_t *kvs_hash_bucket_new(int hint) {
    kvs_hash_bucket_t *b = malloc(sizeof(kvs_hash_bucket_t));
    if (b == NULL) {
        return b;
    }

    b->buckets = calloc(hint, sizeof(void **));
    if (b->buckets == NULL) {
        free(b);
        return NULL;
    }

    b->use  = 0;
    b->size = hint;
    b->mask = hint - 1;
    return b;
}

void kvs_hash_bucket_free(kvs_hash_bucket_t *b) {
    if (b) {
        free((void **)b->buckets);
        free(b);
    }
}

int kvs_hash_next_power(int hint) {
    int i, p;

    if (hint == INT32_MAX) {
        return hint;
    }

    for (i = 2, p = 1; i < 31; i++) {
        if (p >= hint) {
            return p;
        }

        p *= 2;
    }

    return p;
}

kvs_hash_t *kvs_hash_new(int hint) {
    kvs_hash_t *h = NULL;

    if ((h = malloc(sizeof(kvs_hash_t))) == NULL) {
        return NULL;
    }

    if (hint < KVS_INIT_HASH_SIZE) {
        hint = KVS_INIT_HASH_SIZE;
    } else {
        hint = kvs_hash_next_power(hint);
    }

    if ((h->buckets = kvs_hash_bucket_new(hint)) == NULL) {
        goto fail;
    }

    h->oldbuckets = NULL;
    h->hash_func  = default_hash;
    return h;
fail:
    free(h);
    return NULL;
}

kvs_hash_node_t *kvs_hash_node_new(const void *k, int klen, void *v) {
    kvs_hash_node_t *n = malloc(sizeof(kvs_hash_node_t));
    if (n == NULL) {
        return NULL;
    }
    n->key = malloc(klen);
    if (n->key == NULL) {
        free(n);
        return NULL;
    }

    memcpy((void *)n->key, k, klen);
    n->klen = klen;
    n->val  = v;
    n->next = NULL;
    return n;
}

void kvs_hash_node_free(kvs_hash_node_t *n) {
    free((void *)n->key);
    free(n);
}

int kvs_hash_need_expand(kvs_hash_t *h) {
    return 0;
}

kvs_hash_node_t **kvs_hash_find_core(kvs_hash_t *h, const void *k, size_t *klen, unsigned *hash) {
    kvs_hash_node_t **pp;

    pp = NULL;
    /* TODO 找到有用的扩容策略 */
    if (kvs_hash_need_expand(h)) {
    }

    *hash = h->hash_func(k, (int *)klen) & h->buckets->mask ;
    if (h->buckets) {
        pp = &h->buckets->buckets[*hash];
    }

    for (; *pp; pp = &(*pp)->next) {
        if ((*pp)->klen == *klen &&
            !memcmp((*pp)->key, k, *klen)) {

            return pp;
        }
    }

    return NULL;
}

kvs_hash_node_t **kvs_hash_find(kvs_hash_t *h, const void *k, size_t klen) {

    unsigned hash;
    return kvs_hash_find_core(h, k, &klen, &hash);
}

int kvs_hash_add(kvs_hash_t *h, const void *k, size_t klen, void **v) {
    unsigned          hash;
    void             *prev;
    kvs_hash_node_t  *newp;
    kvs_hash_node_t **pp;

    pp = kvs_hash_find_core(h, k, &klen, &hash);
    if (pp && *pp != NULL) {
        /*replace old value */
        prev       = (*pp)->val;
        (*pp)->val = *v;
        *v         = prev;
        return 1;
    }

    newp = kvs_hash_node_new(k, klen, *v);
    if (newp == NULL) {
        return -1;
    }

    newp->next = h->buckets->buckets[hash];
    h->buckets->buckets[hash] = newp;
    return 0;
}

// 返回-1; key不存在
int kvs_hash_del(kvs_hash_t *h, const void *k, size_t klen, void **v) {
    kvs_hash_node_t **pp, *p;

    pp = kvs_hash_find(h, k, klen);
    if (pp == NULL) {
        return -1;
    }

    p   = *pp;
    *pp = p->next;
    *v  = p->val;

    kvs_hash_node_free(p);
    return 0;
}

void kvs_hash_free(kvs_hash_t *h, void (*myfree)(void *)) {
    int i, j;

    kvs_hash_bucket_t *buckets;
    kvs_hash_node_t   *p, *n;

    buckets = h->buckets;
    j       = 2;

    while (buckets && j--) {

        for (i = buckets->size -1; i >= 0; i--) {

            for (p = buckets->buckets[i]; p;) {
                n = p->next;

                myfree(p->val);
                kvs_hash_node_free(p);
                p  = n;
            }
        }

        buckets = h->oldbuckets;
    }

    kvs_hash_bucket_free(h->buckets);
    kvs_hash_bucket_free(h->oldbuckets);
    free(h);
}

void myfree(void *v) {
    free(v);
}

void test_add_del_free() {
    kvs_hash_t *h = kvs_hash_new(800 * 2);
    int   i;
    void *v;
    for (i = 0; i < 800; i++) {

        v = NULL;
        asprintf((char **)&v, "key(int) val is %d", i);
        kvs_hash_add(h, (const void *)&i, sizeof(i), &v);
    }

    v = NULL;
    for (i = 0; i < 800; i++) {
        kvs_hash_del(h, (const void *)&i, sizeof(i), &v);
    }
    kvs_hash_free(h, myfree);
    printf("%s, test ok\n", __func__);
}

void test_repeat_key_add() {
    kvs_hash_t *h = kvs_hash_new(800 * 2);

    int   i, rv;
    void *v;

    for (i = 0; i < 800; i++) {

        v = NULL;
        asprintf((char **)&v, "key(int) val is %d", i);
        kvs_hash_add(h, (const void *)&i, sizeof(i), &v);
    }

    i = 0;
    v = strdup("hello world");
    rv = kvs_hash_add(h, (const void *)&i, sizeof(i), &v);
    printf("add repeat key rv is:%d\n", rv);
    if (rv == 1) {
        printf("old value is:%s\n", (char *)v);
        printf("%s, test ok\n", __func__);
    } else {
        printf("%s, test fail\n", __func__);
    }

    kvs_hash_free(h, myfree);
}

void test_find_key() {
    kvs_hash_t      *h = kvs_hash_new(800 * 2);
    kvs_hash_node_t **pp;

    int   i;
    void *v;

    for (i = 0; i < 800; i++) {

        v = NULL;
        asprintf((char **)&v, "key(int) val is %d", i);
        kvs_hash_add(h, (const void *)&i, sizeof(i), &v);
    }

    i = 0;
    for (i = 0; i < 800; i++) {
        pp = kvs_hash_find(h, (const void *)&i, sizeof(i));
        if (pp == NULL) {
            printf("%s, test fail\n", __func__);
            kvs_hash_free(h, myfree);
            return;
        }
    }

    kvs_hash_free(h, myfree);
    printf("%s, test ok\n", __func__);
}

int main() {
    test_add_del_free();
    test_repeat_key_add();
    test_find_key();
    return 0;
}
