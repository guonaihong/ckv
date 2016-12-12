#include <stdlib.h>
#include <string.h>
#include "kvs_str.h"

kvs_str_t *kvs_str_new(const char *p, size_t len) {
    kvs_str_t *s = malloc(sizeof(kvs_str_t));
    if (s == NULL) {
        return s;
    }

    s->p      = malloc(len + 1);
    s->len    = len;
    memcpy(s->p, p, len);
    s->p[len] = '\0';

    return s;
}

void kvs_str_free(kvs_str_t *s) {
    free(s->p);
    free(s);
}

int kvs_buf_append(kvs_buf_t *b, const char *p, int len) {

    int needlen = kvs_buf_dsize;
    char *newp  = NULL;

    if (b->alloc == 0) {
        if (len > kvs_buf_dsize) {
            needlen = len;
        }

        b->p     = (char *)malloc(needlen + 1);
        b->len   = 0;
        b->alloc = needlen + 1;
    }

    needlen = b->len + len + 1;
    if (needlen > b->alloc) {

        if (needlen < 2 * b->alloc) {
            needlen = 2 * b->alloc;
        }

        newp = realloc(b->p, needlen);
        if (newp == NULL) {
            return -1;
        }
        b->p = newp;
        b->alloc = needlen;
    }

    memcpy(b->p + b->len, p, len);
    b->len += len;
    b->p[b->len] = '\0';
    return 0;
}
