#pragma once

typedef struct linked_list_node
{
    void *data;
    struct linked_list_node *next;
    struct linked_list_node *pre;
} linked_list_node_t;

typedef struct linked_list
{
    int size;
    linked_list_node_t *head;
    linked_list_node_t *tail;
} linked_list_t;

linked_list_t *linked_list_create();
/// @brief 清空并销毁链表中的结构体，但不销毁链表本身
/// @param list 链表指针
/// @param callback_destroy_node 链表接电的销毁毁掉函数，如果不需要对节点进行销毁操作，则传入null
void linked_list_clear(linked_list_t *list, void (*callback_destroy_node)(void *));
/// @brief 销毁链表，包括其中的节点以及链表本身
/// @param list 链表指针
/// @param callback_destroy_node 链表接电的销毁毁掉函数，如果不需要对节点进行销毁操作，则传入null
void linked_list_destroy(linked_list_t *list, void (*callback_destroy_node)(void *));
int linked_list_push_tail(linked_list_t *list, void *node_data);
void *linked_list_pop_head(linked_list_t *list);
void linked_list_remove();