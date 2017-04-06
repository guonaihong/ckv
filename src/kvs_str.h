#ifndef __KVS_STR_H
#define __KVS_STR_H

#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct kvs_buf_t kvs_buf_t;
typedef struct kvs_buffer_t kvs_buffer_t;
typedef struct kvs_str_t kvs_str_t;
typedef struct kvs_strs_t kvs_strs_t;

struct kvs_buf_t {
    char *p;
    int   len;
    int   alloc;
    int   flag;
};

#define kvs_buf_dsize 64

struct kvs_buffer_t {
    char *p;
    int   len;
    int   alloc;
    int   flag;
    char  buf[kvs_buf_dsize];
};

struct kvs_str_t {
    char *p;
    int   len;
};

#define KVS_STRS_ALLOC 0x1

struct kvs_strs_t {
    kvs_str_t *argv;
    int        argc;
    int        alloc;
};

#define IS_BUF 0x0

#define IS_BUFFER 0x1

#define kvs_buf_len(b) (b)->len

#define kvs_buf_reset(b) (b)->len = 0;

#define kvs_buf_null(b) (b)->p = NULL; (b)->len = 0; (b)->alloc = 0; (b)->flag = IS_BUF;

#define kvs_buffer_null(b) kvs_buf_null(b); (b)->flag = IS_BUFFER;

#define kvs_str_null(b) (b)->p = NULL; (b)->len = 0;

#define kvs_str_len(s) kvs_buf_len(s)

kvs_str_t *kvs_str_new(const char *p, size_t len);

void kvs_str_free(kvs_str_t *s);

kvs_buf_t *kvs_buf_new(const char *p, size_t len);

int kvs_buf_append(kvs_buf_t *b, const char *p, int len);

int kvs_buf_append_sprintf(kvs_buffer_t *b, const char *fmt, ...);

int kvs_buf_truncate(kvs_buf_t *b, int n);

int kvs_buf_getrange(kvs_buffer_t *b, int start, int end, kvs_str_t *out);

int kvs_buf_setrange(kvs_buffer_t *b, int offset, const char *p, int len);

void kvs_buf_free(kvs_buf_t *b);

/* strs */
void kvs_strs_init(kvs_strs_t *strs);

kvs_strs_t *kvs_strs_new();

int kvs_strs_split(kvs_strs_t *strs, const char *s, const char *dem, int flags);

void kvs_strs_free(kvs_strs_t *strs);

void kvs_strs_reset(kvs_strs_t *strs);
#ifdef __cplusplus
}
#endif

#endif
