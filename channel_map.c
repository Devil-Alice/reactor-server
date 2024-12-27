#include "channel_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

channel_map_t *channel_map_create()
{
    channel_map_t *channel_map = malloc(sizeof(channel_map_t));
    channel_map->list = linked_list_create();
    return channel_map;
}

void channel_map_destroy(channel_map_t *channel_map)
{
    while (1)
    {
        linked_list_node_t *node = linked_list_pop_head(&(channel_map->list));
        free(node->data); // 释放节点的数据，也就是channel
        if (channel_map->list.head == NULL)
            break; // 释放完毕，退出
    }
}
