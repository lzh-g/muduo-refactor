#pragma once

#include "Poller.h"

#include <vector>
#include <sys/epoll.h>

/**
 * @brief epoll IO多路复用模块类，继承Poller
 */
class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop *loop);
    ~EpollPoller() override;

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize = 16;

    // 填写活跃连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

    // 更新channel，调用epoll_ctl
    void update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_; // epoll_create创建的fd

    EventList events_; // 存放epoll_wait返回的事件集
};
