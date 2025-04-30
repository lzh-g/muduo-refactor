#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;

/**
 * @brief 通道类，用于每个socket连接的事件分发，封装了sockfd及其感兴趣的event如EPOLLIN、EPOLLOUT事件，还绑定了poller返回的具体事件，一个Channel对应一个sockfd
 */
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到Poller通知后处理事件，该方法在EventLoop::loop()中调用
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止当channel被手动remove时，channel还在执行回调操作，用于解决TcpConnection和Channel生命周期时长问题，从而保证Channel对象在TcpConnection销毁前销毁
    void tie(const std::shared_ptr<void> &);

    int fd() const { return fd_; };
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; }

    // 设置fd相应的事件状态，相当于epoll_ctl的add、delete
    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }
    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }
    void disableAll()
    {
        events_ &= kNoneEvent;
        update();
    }

    // 返回fd当前的事件状态
    bool isReading() const { return events_ & kReadEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isNoneEvent() const { return events_ == kNoneEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // one loop per thread
    EventLoop *ownerLoop() { return loop_; }
    /**
     * @brief 在channel所属的EventLoop中将当前channel删除
     */
    void remove();

private:
    /**
     * @brief 当channel中的fd事件events发生改变后，update负责在poller中更改fd事件(epoll_ctl)
     */
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 该channel所属的事件循环
    const int fd_;    // fd，Poller监听的对象
    int events_;      // 注册fd感兴趣的事件类型集合
    int revents_;     // Poller返回的具体发生事件
    int index_;       // channel状态-1/1/2

    std::weak_ptr<void> tie_;
    bool isTied_;

    // channel通道可获知fd最终发生的具体事件events，所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};