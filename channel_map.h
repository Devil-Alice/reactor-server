#include "channel.h"
#include "linked_list.h"

typedef struct channel_map
{
    //链表，内部存放channel*
    linked_list_t list;
} channel_map_t;

channel_map_t *channel_map_create();

void channel_map_destroy(channel_map_t *channel_map);

