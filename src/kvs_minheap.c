#include <stdio.h>
#include "kvs_minheap.h"

int kvs_minheap_init(kvs_minheap_t *minheap) {

    minheap->arr    = (void **)malloc(KVS_MIN_SIZE * sizeof(void *));

    if (minheap->arr == NULL) {
        return -1;
    }

    minheap->pos    = 0;

    minheap->flag   = 0;
    minheap->nalloc = KVS_MIN_SIZE;
    return 0;
}

kvs_minheap_t *kvs_minheap_new() {

    kvs_minheap_t *minheap = malloc(sizeof(kvs_minheap_t));
    if (minheap == NULL) {
        return minheap;
    }

    kvs_minheap_init(minheap);

    minheap->flag = 1;
    return minheap;
}

int kvs_minheap_add(kvs_minheap_t *minheap, void *t) {

    int   parent = KVS_MINHEAP_PARENT(minheap->pos);
    int   child  = minheap->pos;
    void *tmp;

    if (minheap->pos >= minheap->nalloc) {

        minheap->nalloc *= 2;
        tmp = realloc(minheap->arr, minheap->nalloc);

        if (tmp == NULL) {
            return -1;
        }

        minheap->arr = tmp;
    }

    minheap->arr[minheap->pos] = t;

    while (child > 0 &&
           minheap->cmp(minheap->arr[child], minheap->arr[parent]) < 0) {

        tmp = minheap->arr[child];
        minheap->arr[child] = minheap->arr[parent];
        minheap->arr[parent] = tmp;

        child = parent;

        parent = KVS_MINHEAP_PARENT(parent);
    }

    minheap->pos++;

    return 0;
}

int kvs_minheap_del(kvs_minheap_t *minheap, void **t) {

    if (minheap->pos <= 0) {
        return -1;
    }

    int child, parent;
    int tmp;

    *t = minheap->arr[0];


    minheap->arr[0] = minheap->arr[minheap->pos - 1];
    parent  = 0;

    if (minheap->pos - 1 <= minheap->nalloc / 2) {

        minheap->nalloc /= 2;
        tmp = (void **)realloc(minheap->arr, minheap->nalloc);

        if (tmp == NULL) {
            return -1;
        }

        minheap->arr = tmp;
    }

    while (parent < minheap->pos) {

        child = KVS_MINHEAP_LEFT(parent);

        //printf("right(%d) pos(%d)\n", KVS_MINHEAP_RIGHT(parent), minheap->pos);
        if (minheap->cmp(minheap->arr[KVS_MINHEAP_RIGHT(parent)],
                         minheap->arr[KVS_MINHEAP_LEFT(parent)]) < 0) {

            child = KVS_MINHEAP_RIGHT(parent);
        }

        if (minheap->cmp(minheap->arr[child], minheap->arr[parent]) < 0) {
            tmp = minheap->arr[child];
            minheap->arr[child] = minheap->arr[parent];
            minheap->arr[parent] = tmp;

            parent = child;
        } else {
            break;
        }
    }

    minheap->pos--;
    return 0;
}

void kvs_minheap_free(kvs_minheap_t *minheap) {

    int i, len;

    for (i = 0, len = minheap->pos; i < len; i++) {
        if (minheap->myfree) {
            minheap->myfree(minheap->arr[i]);
        }
    }

    free(minheap->arr);
    if (minheap->flag = 1) {
        free(minheap);
    }
}
