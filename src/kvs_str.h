#ifndef __KVS_STR_H
#define __KVS_STR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kvs_buf_t kvs_buf_t;
typedef struct kvs_str_t kvs_str_t;

struct kvs_buf_t {
    char *p;
    int   len;
    int   alloc;
};

struct kvs_str_t {
    char *p;
    int   len;
};

#define kvs_buf_dsize 512
#define kvs_buf_len(b) b->len
#define kvs_buf_null(b) (b)->p = NULL; (b)->len = 0; (b)->alloc = 0;

#define kvs_str_len(s) kvs_buf_len(s)

int kvs_buf_append(kvs_buf_t *b, const char *p, int len);

#ifdef __cplusplus
}
#endif

#endif
