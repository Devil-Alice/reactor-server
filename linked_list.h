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

linked_list_t linked_list_create();

void linked_list_destroy(linked_list_t *list);

int linked_list_push_tail(linked_list_t *list, void *node_data);

void *linked_list_pop_head(linked_list_t *list);

void linked_list_remove();