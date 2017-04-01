#include <stdio.h>
#include <stdlib.h>
#include "kvs_minheap4.h"

int kvs_minheap_init(kvs_minheap_t *minheap) {

    minheap->t    = (void **)malloc(KVS_MIN_SIZE * sizeof(void *));

    if (minheap->t == NULL) {
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

    int   p = 0;
    int   c = minheap->pos;
    void *tmp;

    if (minheap->pos >= minheap->nalloc) {

        minheap->nalloc *= 2;
        tmp = realloc(minheap->t, minheap->nalloc);

        if (tmp == NULL) {
            return -1;
        }

        minheap->t = tmp;
    }

    tmp = t;
    minheap->t[minheap->pos] = t;

    while (c > 0) {
        p = KVS_MINHEAP_PARENT(c);

        if (minheap->cmp(minheap->t[c], minheap->t[p]) >= 0) {
            break;
        }

        minheap->t[c] = minheap->t[p];
        minheap->t[p] = tmp;

        c = p;
    }

    minheap->pos++;

    return 0;
}

int kvs_minheap_del(kvs_minheap_t *minheap, void **tv) {

    if (minheap->pos <= 0) {
        return -1;
    }

    int c, c3, p, n;
    void *tmp, *min, *min3;
    void **t;

    *tv = minheap->t[0];

    minheap->t[0] = minheap->t[minheap->pos - 1];
    p = 0;

    if (minheap->pos - 1 <= minheap->nalloc / 2) {

        minheap->nalloc /= 2;
        tmp = (void **)realloc(minheap->t, minheap->nalloc);

        if (tmp == NULL) {
            return -1;
        }

        minheap->t = tmp;
    }

    tmp = minheap->t[p];
    n = minheap->pos;
    t = minheap->t;
    for (;;) {

        c = KVS_MINHEAP_LEFT(p);
        c3 = c + 2;

        if (c >= n) {
            break;
        }

        min = t[c];
        if ((c + 1) < n && minheap->cmp(t[c+1], t[c]) < 0) {
            min = t[c+1];
            c++;
        }

        if (c3 < n) {
            min3 = t[c3];
            if (c3 + 1 < n && minheap->cmp(t[c3+1], t[c3]) < 0) {
                min3 = t[c3+1];
                c3++;
            }

            if (minheap->cmp(min3, min) < 0) {
                min = min3;
                c = c3;
            }
        }

        if (minheap->cmp(min, tmp) >= 0) {
            break;
        }

        t[p] = t[c];
        t[c] = tmp;
        p    = c;
    }

    minheap->pos--;
    return 0;
}

void kvs_minheap_free(kvs_minheap_t *minheap) {

    int i, len;

    for (i = 0, len = minheap->pos; i < len; i++) {
        if (minheap->myfree) {
            minheap->myfree(minheap->t[i]);
        }
    }

    free(minheap->t);
    if (minheap->flag == 1) {
        free(minheap);
    }
}
