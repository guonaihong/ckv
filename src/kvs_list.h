#ifndef __KVS_LIST_H
#define __KVS_LIST_H

#if __cplusplus
extern "C" {
#endif


    typedef struct kvs_list_node_t kvs_list_node_t;
    typedef struct kvs_list_t      kvs_list_t;

    struct kvs_list_node_t {
        struct kvs_list_node_t *prev;
        struct kvs_list_node_t *next;
        void                   *val;
    };

    struct kvs_list_t {
        struct kvs_list_node_t head;
        int                    len;
    };

void kvs_list_init(kvs_list_t *list);

kvs_list_t *kvs_list_new(void);

int kvs_list_add(kvs_list_t *list, void *val);

int kvs_list_add_tail(kvs_list_t *list, void *val);

int kvs_list_del(kvs_list_t *list, kvs_list_node_t *n);

#define kvs_list_head_init(head) \
 do {(head)->prev = head; (head)->next = head;}while(0)

#define kvs_list_for_each(list, node) \
 for(node = (list)->next; node != (list); node = node->next)

#define kvs_list_for_each_safe(list, p, q) \
 for(p = (list)->next, q = p->next; p != list; p = q, q = p->next)

#if __cplusplus
}
#endif

#endif
