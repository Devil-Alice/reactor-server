#pragma once
#include "channel.h"
#include "linked_list.h"

//channel_map类是fd与channel的映射，fd即是数组索引，数组的元素就是channel
typedef struct channel_map
{
    // 数组，内部存放channel*
    channel_t **list;
    int capacity;
} channel_map_t;

channel_map_t *channel_map_create(int capacity);
int channel_map_expand(channel_map_t *channel_map, int capacity);
int channel_map_add(channel_map_t *channel_map, int fd, channel_t *channel);
int channel_map_remove(channel_map_t *channel_map, int fd);
void channel_map_destroy(channel_map_t *channel_map);
