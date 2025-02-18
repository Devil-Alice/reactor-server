#include "channel.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>

CHANNEL_EVENT operator|(CHANNEL_EVENT l_hand_side, CHANNEL_EVENT r_hand_side)
{
    return static_cast<CHANNEL_EVENT>(static_cast<int>(l_hand_side) | static_cast<int>(r_hand_side));
}

CHANNEL_EVENT operator&(CHANNEL_EVENT l_hand_side, CHANNEL_EVENT r_hand_side)
{
    return static_cast<CHANNEL_EVENT>(static_cast<int>(l_hand_side) & static_cast<int>(r_hand_side));
}


Channel::Channel(int fd, CHANNEL_EVENT events, channel_handle_func rcallback, channel_handle_func wcallback, channel_handle_func dcallback, void *args)
{
    this->fd = fd;
    this->events = events;
    this->read_callback = rcallback;
    this->write_callback = wcallback;
    this->destroy_callback = dcallback;
    this->args = args;
}

Channel::~Channel()
{
    // 这里的args交给销毁回调释放
    //  if (channel->args != NULL)
    //      free(channel->args);

}

void Channel::enable_write_event(bool flag)
{
    if (flag == true)
        events = static_cast<CHANNEL_EVENT>(static_cast<int>(events) | static_cast<int>(CHANNEL_EVENT::WRITE));
    else // events &= ~static_cast<int>(CHANNEL_EVENT::WRITE);
        events = static_cast<CHANNEL_EVENT>(static_cast<int>(events) & ~(static_cast<int>(CHANNEL_EVENT::WRITE)));
    return;
}

bool Channel::get_write_event_status()
{
    return static_cast<int>(events) & static_cast<int>(CHANNEL_EVENT::WRITE);
}
