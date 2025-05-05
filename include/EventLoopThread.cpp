#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    : loop_(nullptr), exiting_(false), thread_(std::bind(&EventLoopThread::threadFunc, this), name), mutex_(), cond_(), callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop()
{
    thread_.start(); // 启用Thread类对象中的start创建线程
    EventLoop *loop = nullptr;

    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this]()
               { return loop_ != nullptr; }); // 谓词形式，loop_不为空则线程继续执行
    loop = loop_;

    return loop;
}

void EventLoopThread::threadFunc()
{
    // EventLoopThread创建时，构造一个EventLoop对象，EventLoop构造函数中初始化t_loopInThisThread为this
    EventLoop loop; // 创建一个独立的EventLoop对象，与上面的线程一一对应，即one loop per thread

    if (callback_)
    {
        callback_(&loop);
    }

    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = &loop;
    cond_.notify_one();

    loop.loop(); // 执行EventLoop的loop()，开启底层Poller的poll()
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}
