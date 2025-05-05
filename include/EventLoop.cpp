#include "EventLoop.h"
#include "Channel.h"
#include "Logger.h"
#include "Poller.h"

#include <errno.h>
#include <fcntl.h>
#include <memory>
#include <sys/eventfd.h>
#include <unistd.h>

// one thread one loop 防止一个线程创建多个EventLoop
__thread EventLoop *t_loopInThisThread = nullptr;

// 默认Poller IO复用接口超时时间
const int kPollTimeMs = 10000; // 10s

// 创建wakeupfd 唤醒subLoop处理新的channel
int createEventfd()
{
    int evfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evfd < 0)
    {
        LOG_FATAL("eventfd error: %d\n", errno);
    }
    return evfd;
}

EventLoop::EventLoop()
    : looping_(false), quit_(false), callingPendingFunctors_(false), threadId_(CurrentThread::tid()), poller_(Poller::newDefaultPoller(this)), wakeupFd_(createEventfd()), wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d\n", this, threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exist in this thread %d\n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping\n", this);

    while (!quit)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_)
        {
            // 通知channel处理相应事件
            channel->handleEvent(pollReturnTime_);
        }

        /**
         * 执行当前EventLoop需要处理的回调操作
         * 多线程环境下，mainloop主要工作为：
         * accept接收连接 => 将accept返回的connfd打包为channel => TcpServer::newConnection通过轮询将TcpConnection对象分配给subloop处理
         *
         * mainloop调用queueInLoop将回调传入subloop，queueInLoop通过wakeup唤醒subloop
         */
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping.\n", this);
    looping_ = true;
}

void EventLoop::quit()
{
    /**
     * 修改quit_表示停止loop内的循环，但可能会阻塞在poll()处，所以需要调用wakeup向wakeFd_写入数据解除阻塞
     */
    quit_ = true;

    if (!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{
    // 当前EventLoop中执行回调
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        // 非当前EventLoop中执行cb，需要唤醒其Loop所在线程执行
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    std::unique_lock<std::mutex> lock(mutex_);
    pendingFunctors_.emplace_back(cb);

    /**
     * 非当前EventLoop或pendingFunctors中有新的回调待处理，通过wakeup唤醒相应的loop线程执行回调
     */
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    size_t n = read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8\n", n);
    }
}

void EventLoop::wakeup()
{
    // 向wakeupFd_写数据，wakeupChannel发生读事件，其loop线程被唤醒
    uint64_t one = 1;
    size_t n = write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n", n);
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    std::unique_lock<std::mutex> lock(mutex_);
    functors.swap(pendingFunctors_);

    for (const Functor &functor : functors)
    {
        functor();
    }

    callingPendingFunctors_ = false;
}
