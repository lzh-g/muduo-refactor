#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class EventLoop;
class Channel; // Poller类不拥有Channel对象，根据最小化头文件包含原则，头文件中使用前向声明，源文件包含具体头文件

/**
 * @brief IO多路复用模块类，管理epollfd
 */
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    // IO复用统一接口，在EventLoop中调用
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChanels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // 判断channel是否在当前Poller中
    bool hasChannel(Channel *channel) const;

    static Poller *newDefaultPoller(EventLoop *loop);

    void assertInLoopThread() const;

protected:
    // sockfd: channel
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;

private:
    EventLoop *ownerLoop_; // Poller所属的事件循环
};
