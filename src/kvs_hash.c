#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
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

    h->expand_buckets = NULL;
    h->hash_func      = default_hash;
    h->index          = KVS_UNUSED;
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
    /* is expanding */
    if (h->expand_buckets != NULL) {
        return 0;
    }

    if (h->buckets->use > h->buckets->size) {
        return 1;
    }
    return 0;
}

int kvs_hash_rehash(kvs_hash_t *h, int nbucket) {
    kvs_hash_node_t *p, *n;
    unsigned         newhash;
    if (h->expand_buckets == NULL) {
        return 0;
    }

    if (h->buckets->use == 0) {

        kvs_hash_bucket_free(h->buckets);
        h->buckets        = h->expand_buckets;
        h->expand_buckets = NULL;
        h->index          = KVS_UNUSED;
        return 0;
    }

    if (h->index == KVS_UNUSED) {
        h->index = h->buckets->size - 1;
    }

    for (; h->index >= 0 && nbucket > 0; h->index--) {

        for (p = h->buckets->buckets[h->index]; p;) {
            n = p->next;
            newhash = h->hash_func(p->key, &p->klen) & h->expand_buckets->mask;

            p->next = h->expand_buckets->buckets[newhash];
            h->expand_buckets->buckets[newhash] = p;

            h->buckets->use--;
            h->expand_buckets->use++;

            p = n;
        }

        nbucket--;
    }

    return 0;
}

kvs_hash_node_t **kvs_hash_find_core(kvs_hash_t *h,
                                     const void *k,
                                     size_t     *klen,
                                     unsigned   *hash,
                                     unsigned char *whence) {
    kvs_hash_node_t **pp;
    int               i;
    unsigned          hv;

    pp = NULL;

    kvs_hash_rehash(h, 10);

    kvs_hash_bucket_t *buckets = h->buckets;


    for (i = 2; i--; ) {
        hv = h->hash_func(k, (int *)klen) & buckets->mask;
        if (hash) {
            *hash = hv;
        }

        for (pp = &buckets->buckets[hv]; *pp; pp = &(*pp)->next) {
            if ((*pp)->klen == *klen &&
                    !memcmp((*pp)->key, k, *klen)) {

                return pp;
            }
        }

        if (h->expand_buckets) {
            buckets = h->expand_buckets;
            if (whence) {
                *whence = KVS_EXPAND;
            }
        }
    }

    return NULL;
}

int kvs_hash_resize(kvs_hash_t *h, int new_size) {
    int hint;

    if (h->index != KVS_UNUSED) {
        return -1;
    }

    hint = kvs_hash_next_power(new_size);
    h->expand_buckets = kvs_hash_bucket_new(hint);
#if 0
    printf("old bucket use(%d) size(%d): new bucket size is %d: hint is %d new buckets is %d\n",
            h->buckets->use, h->buckets->size, h->expand_buckets->size, hint, h->expand_buckets->mask);
#endif
    return 0;
}

kvs_hash_node_t *kvs_hash_find(kvs_hash_t *h, const void *k, size_t klen) {

    kvs_hash_node_t **pp;
    pp = kvs_hash_find_core(h, k, &klen, NULL, NULL);
    if (pp != NULL) {
        return *pp;
    }
    return NULL;
}

int kvs_hash_add(kvs_hash_t *h, const void *k, size_t klen, void **v) {
    unsigned          hash;
    void             *prev;
    kvs_hash_node_t  *newp;
    kvs_hash_node_t **pp;

    if (kvs_hash_need_expand(h)) {
        assert(h->expand_buckets == NULL);

        kvs_hash_resize(h, h->buckets->size * 2);


    }

    pp = kvs_hash_find_core(h, k, &klen, &hash, NULL);
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

    kvs_hash_bucket_t *buckets = h->expand_buckets;
    if (buckets == NULL) {
        buckets = h->buckets;
    }

    newp->next = buckets->buckets[hash];
    buckets->buckets[hash] = newp;
    buckets->use++;
    return 0;
}

// 返回-1; key不存在
int kvs_hash_del(kvs_hash_t *h, const void *k, size_t klen, void **v) {
    kvs_hash_node_t **pp, *p;
    unsigned char whence = 0;
    
    pp = kvs_hash_find_core(h, k, &klen, NULL, &whence);
    if (pp == NULL) {
        return -1;
    }

    p   = *pp;
    *pp = p->next;
    *v  = p->val;

    whence == KVS_EXPAND ? h->expand_buckets->use-- : h->buckets->use--;
    kvs_hash_node_free(p);
    return 0;
}

void kvs_hash_free(kvs_hash_t *h, void (*myfree)(void *)) {
    int i, j;

    kvs_hash_bucket_t *buckets;
    kvs_hash_node_t   *p, *n;

    buckets = h->buckets;
    j       = 2;

    while (j--) {

        if (buckets == NULL) {
            continue;
        }

        for (i = buckets->size -1; i >= 0; i--) {

            for (p = buckets->buckets[i]; p;) {
                n = p->next;

                myfree(p->val);
                kvs_hash_node_free(p);
                p  = n;
            }
        }

        buckets = h->expand_buckets;
    }

#if 0
    printf("buckets use(%d):expand_buckets use(%d)\n",
            h->buckets ? h->buckets->use : 0,
            h->expand_buckets ? h->expand_buckets->use : 0);
#endif

    kvs_hash_bucket_free(h->buckets);
    kvs_hash_bucket_free(h->expand_buckets);
    free(h);
}

#if 0
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
        free(v);
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

    i = -30;
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
    kvs_hash_node_t *p;

    int   i;
    void *v;

    for (i = 0; i < 800; i++) {

        v = NULL;
        asprintf((char **)&v, "key(int) val is %d", i);
        kvs_hash_add(h, (const void *)&i, sizeof(i), &v);
    }

    i = 0;
    for (i = 0; i < 800; i++) {
        p = kvs_hash_find(h, (const void *)&i, sizeof(i));
        if (p == NULL) {
            printf("%s, test fail\n", __func__);
            kvs_hash_free(h, myfree);
            return;
        }
    }

    kvs_hash_free(h, myfree);
    printf("%s, test ok\n", __func__);
}

void test_hash_expand() {

    kvs_hash_t      *h = kvs_hash_new(4);
    kvs_hash_node_t *p = NULL;
    int   i;
    void *v;
    int   max = 10000000;

    for (i = 0; i < max; i++) {
        v = NULL;
        asprintf((char **)&v, "test expand:key(int) val is %d", i);
        kvs_hash_add(h, (const void *)&i, sizeof(i), &v);
    }

    for (i = 0; i < max; i++) {
        v = NULL;
        asprintf((char **)&v, "test expand:key(int) val is %d", i);
        p = kvs_hash_find(h, (const void *)&i, sizeof(i));
        if (p == NULL) {
            printf("test %s fail\n", __func__);
            assert(p != NULL);
        }
    }

    for (i = 0; i < max; i++) {
        v = NULL;
        asprintf((char **)&v, "test expand:key(int) val is %d", i);
        if (kvs_hash_del(h, (const void *)&i, sizeof(i), &v) == 0) {
            free(v);
        }
    }
    kvs_hash_free(h, myfree);
}

int main() {
    //test_add_del_free();
    //test_repeat_key_add();
    //test_find_key();
    test_hash_expand();
    return 0;
}
#endif
