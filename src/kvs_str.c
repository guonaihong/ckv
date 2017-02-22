#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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

int kvs_buf_truncate(kvs_buf_t *b, int n) {
    kvs_buffer_t *buffer;

    buffer = (kvs_buffer_t *)b;

    if (n < 0) {
        return -1;
    }

    if (buffer->flag == IS_BUFFER && buffer->p == buffer->buf) {
        if (n > (long)sizeof(buffer->buf)) {
            return -1;
        }

    }

    memmove(b->p, b->p + n, buffer->len - n);
    buffer->len -= n;
    return 0;
}

int kvs_buf_setrange(kvs_buffer_t *b, int offset, const char *p, int len) {
    int           needlen;
    char         *np;
    kvs_buffer_t *buffer;

    if (offset < 0) {
        return -1;
    }

    needlen = kvs_buf_dsize > offset + len + 1 ? kvs_buf_dsize : offset + len + 1;
    buffer  = (kvs_buffer_t *)b;

    if (b->p == NULL) {

        b->len   = 0;
        b->alloc = needlen;

        if (b->flag == IS_BUFFER) {

            b->p     = buffer->buf;
            b->alloc = sizeof(buffer->buf);

        } else {

            b->p     = (char *)malloc(needlen);
        }
    }

    //needlen = b->len + len + 1;
    needlen = offset + len + 1;
    if (needlen > b->alloc) {

        if (needlen < 2 * b->alloc) {
            needlen = 2 * b->alloc;
        }

        if (b->flag == IS_BUFFER && b->p == buffer->buf) {
            np = malloc(needlen);
            if (np == NULL) {
                return -1;
            }

            memcpy(np, buffer->buf, b->len);
        } else {

            np = realloc(b->p, needlen);
            if (np == NULL) {
                return -1;
            }
        }

        b->p = np;
        b->alloc = needlen;
    }

    memcpy(b->p + offset, p, len);

    if(b->len < offset) {
        memset(b->p + b->len, '\0', offset - b->len);
    }

    b->len = offset + len;
    b->p[offset + len] = '\0';
    //b->p[b->len] = '\0';
    return 0;
}

int kvs_buf_append(kvs_buf_t *b, const char *p, int len) {
    return kvs_buf_setrange((kvs_buffer_t *)b, b->len, p, len);
}

int kvs_buf_append_sprintf(kvs_buffer_t *b, const char *fmt, ...) {

    int     n;
    char   *np;
    va_list ap;

    if (b->p == NULL) {

        b->len   = 0;
        if (b->flag == IS_BUFFER) {
            b->p     = b->buf;
            b->alloc = sizeof(b->buf);
        } else {
            b->p     = malloc(kvs_buf_dsize);
            b->alloc = kvs_buf_dsize;

            if (b->p == NULL) {
                return -1;
            }
        }
    }

    while (1) {
        va_start(ap, fmt);
        n = vsnprintf(b->p + b->len, b->alloc - b->len, fmt, ap);
        va_end(ap);

        if (n > -1 && n < b->alloc) {
            b->len += n;
            return n;
        }

        if (n > -1) {
            b->alloc = b->len + n + 1;
        } else {
            b->alloc = (b->len + n)* 2;
        }

        if (b->flag == IS_BUFFER && b->p == b->buf) {
            b->p = malloc(b->alloc);
            if (b->p == NULL) {
                return -1;
            }
            memcpy(b->p, b->buf, b->len);
        } else {
            if ((np = realloc(b->p, b->alloc)) == NULL) {
                return -1;
            } else {
                b->p = np;
            }
        }
    }

    return 0;
}

int kvs_buf_getrange(kvs_buffer_t *b, int start, int end, kvs_str_t *out) {

    if (start < 0) {
        start = b->len + start;
    }

    if (end < 0) {
        end   = b->len + end;
    }

    if (start > end) {
        goto empty;
    }

    if (start >= b->len) {
        goto empty;
    }

    if (end >= b->len) {
        end = b->len - 1;
    }

    out->len = end - start + 1;
    //TODO: Better memory usage strategy
    out->p   = malloc(out->len + 1);
    if (out->p == NULL) {
        return -1;
    }

    memcpy(out->p, b->p + start, out->len);
    out->p[out->len] = '\0';
    return 0;

empty:
    out->p   = strdup("");
    out->len = 1;
    return 0;
}

void kvs_buf_free(kvs_buf_t *b) {
    kvs_buffer_t *buffer;

    buffer = (kvs_buffer_t *)b;

    if (buffer->flag == IS_BUFFER && buffer->p == buffer->buf) {
        return;
    }

    free(buffer->p);
}
