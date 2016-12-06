#include <stdlib.h>
#include <string.h>
#include "kvs_str.h"

int kvs_buf_append(kvs_buf_t *b, const char *p, int len) {

    int needlen = kvs_buf_dsize;
    char *newp  = NULL;

    if (b->alloc == 0) {
        if (len > kvs_buf_dsize) {
            needlen = len;
        }

        b->p     = (char *)malloc(needlen);
        b->len   = 0;
        b->alloc = needlen;
    }

    needlen = b->len + len;
    if (needlen > b->alloc) {
        needlen = b->alloc * 2;
        if (b->len + len > needlen ) {
            needlen = b->len + len;
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
    return 0;
}
