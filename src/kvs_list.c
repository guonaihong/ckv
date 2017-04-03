#include <stdlib.h>
#include "kvs_list.h"

void kvs_list_init(kvs_list_t *list) {
    list->len = 0;
    kvs_list_head_init(&list->head);
}

kvs_list_t *kvs_list_new(void) {
    kvs_list_t *list;

    list = (kvs_list_t *)malloc(sizeof(kvs_list_t));
    if (list == NULL) {
        return NULL;
    }

    kvs_list_init(list);
    return 0;
}

static void kvs_list_add_core(kvs_list_node_t *prev,
                       kvs_list_node_t *next,
                       kvs_list_node_t *newp) {
    prev->next = newp;
    newp->prev = prev;
    newp->next = next;
    next->prev = newp;
}

static void kvs_list_del_core(kvs_list_node_t *prev,
                       kvs_list_node_t *next) {
    prev->next = next;
    next->prev = prev;
}

static kvs_list_node_t *kvs_list_node_new(void *val) {
    kvs_list_node_t *n;

    n = (kvs_list_node_t *)malloc(sizeof(kvs_list_node_t));
    if (n == NULL) {
        return NULL;
    }

    n->val = val;
    return n;
}

int kvs_list_add(kvs_list_t *list, void *val) {

    kvs_list_node_t *n;

    n = kvs_list_node_new(val);
    if (n == NULL) {
        return -1;
    }

    kvs_list_add_core(&list->head, list->head.next, n);
    return 0;
}

int kvs_list_add_tail(kvs_list_t *list, void *val) {

    kvs_list_node_t *n;

    n = kvs_list_node_new(val);
    if (n == NULL) {
        return -1;
    }

    kvs_list_add_core(list->head.prev, &list->head, n);
    //kvs_list_add_core(&list->head, list->head.prev, n);
    return 0;
}

int kvs_list_del(kvs_list_t *list, kvs_list_node_t *n) {
    kvs_list_del_core(n->prev, n->next);
    return 0;
}

void kvs_list_free(kvs_list_t *list) {
}
