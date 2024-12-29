#include "channel_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

channel_map_t *channel_map_create(int capacity)
{
    channel_map_t *channel_map = malloc(sizeof(channel_map_t));
    channel_map->list = calloc(capacity, sizeof(channel_t));
    return channel_map;
}

int channel_map_expand(channel_map_t *channel_map, int capacity)
{
    channel_map->list = realloc(channel_map->list, capacity * sizeof(channel_t));
    channel_map->capacity = capacity;
    return 0;
}

int channel_map_add(channel_map_t *channel_map, int fd, channel_t *channel)
{
    //检查数组的空间是否足够存放下标位fd的channel
    while (fd > (channel_map->capacity -1))
    {
        channel_map_expand(channel_map, channel_map->capacity * 2);
    }

    channel_map->list[fd] = channel;
    return 0;
}

void channel_map_destroy(channel_map_t *channel_map)
{
    free(channel_map->list);
    free(channel_map);
}
