#include "linked_list.h"
#include <stdio.h>
#include <stdlib.h>

linked_list linked_list_create()
{
    linked_list list = {
        .size = 0,
        .head = NULL,
        .tail = NULL};
    return list;
}

void linked_list_destroy(linked_list *list)
{
    linked_list_node *n = list->head;
    linked_list_node *tmpn;

    while (n != NULL)
    {
        tmpn = n;
        n = n->next;
        free(tmpn);
    }

    list->size = 0;
    list->head = NULL;
    list->tail = NULL;

    return;
}

int linked_list_push_tail(linked_list *list, void *node_data)
{
    // 构造node
    linked_list_node *node = malloc(sizeof(linked_list_node));
    node->data = node_data;
    node->pre = list->tail;
    node->next = NULL;

    // 如果头是空的，则直接给头赋值
    if (list->head == NULL)
        list->head = node;
    else
        list->tail->next = node; // 如果头不是空的，则需要修改尾部的next

    // 直接将node赋值给尾部，链表的大小++
    list->tail = node;
    list->size++;

    return 0;
}

void *linked_list_pop_head(linked_list *list)
{
    // 检查list
    if (list->head == NULL)
        return NULL;

    // 先取出头部节点以及数据
    linked_list_node *n = list->head;
    void *data = n->data;

    // 如果只有一个节点，则需要将tail指向空
    if (list->head == list->tail)
        list->tail = NULL;
    else
        n->next->pre = NULL;

    list->head = n->next;
    free(n);
    list->size--;

    return data;
}
