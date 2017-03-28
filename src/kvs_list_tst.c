#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "kvs_list.h"

static void list_add_tst() {
    printf("list add start\n");
    kvs_list_t list;

    kvs_list_init(&list);

    int i;
    for (i = 1; i < 100; i++) {
        kvs_list_add(&list, (void *)(intptr_t)i);
    }

    kvs_list_node_t *n = NULL;
    kvs_list_for_each(&list.head, n) {
        printf("%ld\n", (intptr_t)n->val);
    }
    //TODO free
}

static void list_add_tail_tst() {

    printf("list add tail\n");
    kvs_list_t list;

    kvs_list_init(&list);

    int i;

    for (i = 1; i < 100; i++) {
        kvs_list_add_tail(&list, (void *)(intptr_t)i);

    }
    kvs_list_node_t *n = NULL;

    kvs_list_for_each(&list.head, n) {
        printf("%ld\n", (intptr_t)n->val);
    }
    //TODO free
}

static void list_del_tst() {
    printf("list del\n");
    kvs_list_t list;

    kvs_list_init(&list);

    int i;
    for (i = 1; i < 100; i++) {
        kvs_list_add(&list, (void *)(intptr_t)i);
    }

    kvs_list_node_t *n = NULL;
    kvs_list_for_each(&list.head, n) {
        printf("add foreach:%ld\n", (intptr_t)n->val);
    }

    kvs_list_node_t *tmp;

    kvs_list_for_each_safe(&list.head, n, tmp) {
        kvs_list_del(&list, n);
        free(n);
    }

    kvs_list_for_each(&list.head, n) {
        printf("del foreach:%ld\n", (intptr_t)n->val);
    }
}

int main() {
    list_add_tst();
    list_add_tail_tst();
    list_del_tst();
    return 0;
}
