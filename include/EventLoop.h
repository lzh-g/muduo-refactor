#pragma once

#include "CurrentThread.h"
#include "Timestamp.h"
#include "noncopyable.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

class Channel;
class Poller;

/**
 * @brief 事件循环类，控制Poller，管理Channel
 */
class EventLoop : public noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();

    // 退出事件循环
    void quit();

    // poll返回的时间戳
    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // 在当前loop中执行cb
    void runInLoop(Functor cb);

    // 将上层注册的回调函数cb放入队列中，唤醒loop所在的线程执行cb
    void queueInLoop(Functor cb);

    // 通过eventfd唤醒loop所在的线程
    void wakeup();

    // EventLoop => Poller
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断EventLoop对象是否在自己的线程中
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

private:
    // 给eventfd返回的文件描述符wakeupFd_绑定的事件回调，当有事件发生时(wakeup)，调用handleRead读wakeupFd_的8字节，并唤醒epoll_wait
    void handleRead();

    void doPendingFunctors(); // 执行回调

    using ChannelList = std::vector<Channel *>;

    std::atomic_bool looping_; // 原子操作 底层通过CAS(compare and swap)实现

    std::atomic_bool quit_; // 标识退出loop循环

    const pid_t threadId_; // 记录当前EventLoop所属线程id

    Timestamp pollReturnTime_; // Poller返回发生事件的Channel时间点

    std::unique_ptr<Poller> poller_;

    int wakeupFd_; // mainLoop获取一个新的Channel，通过轮询法选择一个subLoop，通过该成员变量唤醒subLoop处理Channel

    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_; // 有事件发生的Channel列表，由Poller检测到并填充

    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作

    std::vector<Functor> pendingFunctors_; // 存储loo需要执行的所有回调操作

    std::mutex mutex_; // 保护pendingFunctors_线程安全操作
};
