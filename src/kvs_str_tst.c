#include <stdlib.h>
#include "kvs_str.h"

#define TEST_STR "hello, my friend"

#define TEST_STR2 "hello world"
void getrange_tst() {
    kvs_buffer_t buf;
    kvs_buffer_null(&buf);

    kvs_buf_append((kvs_buf_t *)&buf, TEST_STR, sizeof(TEST_STR) -1);

    kvs_str_t out;
    kvs_buf_getrange(&buf, 0, 4, &out);
    printf("0:-4#(%s)\n", out.p);
    free(out.p);

    kvs_buf_getrange(&buf, -1, -5, &out);
    printf("-1:-5(%s)\n", out.p);
    free(out.p);

    kvs_buf_getrange(&buf, -3, -1, &out);
    printf("-3:-1(%s)\n", out.p);
    free(out.p);

    kvs_buf_getrange(&buf, 0, -1, &out);
    printf("0:-1(%s)\n", out.p);
    free(out.p);

    kvs_buf_getrange(&buf, 0, 1008611, &out);
    printf("0:1008611(%s)\n", out.p);
    free(out.p);
}

void setrange_tst() {

    kvs_buffer_t buf;
    kvs_buffer_null(&buf);

    kvs_buf_append((kvs_buf_t *)&buf, TEST_STR2, sizeof(TEST_STR2) -1);

    kvs_buf_setrange(&buf, 6, "my kvs", sizeof("my kvs") -1);
    printf("set offset 6:(%s)(l:%d)(a:%d)\n", buf.p, buf.len, buf.alloc);

    kvs_buf_setrange(&buf, 0, "hello", sizeof("hello") -1);
    printf("set offset 0:(%s)(l:%d)(a:%d)\n", buf.p, buf.len, buf.alloc);

    kvs_buf_setrange(&buf, 7, "world", sizeof("world") -1);
    printf("set offset 0:(%s)(l:%d)(a:%d)\n", buf.p, buf.len, buf.alloc);

    int i = 0;
    for (i = 0; i < buf.len; i++) {
        printf("(%c:%d)", buf.p[i], buf.p[i]);
    }
    printf("\n");
    kvs_buf_free((kvs_buf_t *)&buf);
}

int main() {

    getrange_tst();

    setrange_tst();
    return 0;
}
