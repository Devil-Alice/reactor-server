#include "channel_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

channel_map_t *channel_map_create(int capacity)
{
    channel_map_t *channel_map = malloc(sizeof(channel_map_t));
    channel_map->list = (channel_t **)calloc(capacity, sizeof(channel_t*));
    channel_map->capacity = capacity;
    return channel_map;
}

int channel_map_expand(channel_map_t *channel_map, int capacity)
{
    channel_t **new_list = (channel_t **)realloc(channel_map->list, capacity * sizeof(channel_t*));
    if (new_list == NULL)
    {
        perror("channel_map_expand realloc");
        return -1;
    }
    channel_map->list = new_list;
    channel_map->capacity = capacity;
    return 0;
}

int channel_map_add(channel_map_t *channel_map, int fd, channel_t *channel)
{
    // 检查数组的空间是否足够存放下标位fd的channel
    while (fd > (channel_map->capacity - 1))
    {
        int ret = channel_map_expand(channel_map, channel_map->capacity * 2);
        if (ret == -1)
        {
            perror("channel_map_add");
            return -1;
        }
    }

    channel_map->list[fd] = channel;
    return 0;
}

int channel_map_remove(channel_map_t *channel_map, int fd)
{
    if (fd > (channel_map->capacity - 1) || channel_map->list[fd] == NULL)
    {
        perror("channel_map_remove");
        return -1;
    }

    free(channel_map->list[fd]);
    channel_map->list[fd] == NULL;

    return 0;
}

void channel_map_destroy(channel_map_t *channel_map)
{
    free(channel_map->list);
    free(channel_map);
}
