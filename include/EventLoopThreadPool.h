#pragma once

#include "noncopyable.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : public noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    // 若工作在多线程中，baseLoop_(mainLoop)以轮询方式分配Channel给subLoop
    EventLoop *getNextLoop();

    // 获取所有的EventLoop
    std::vector<EventLoop *> getAllLoops();

    // 是否启动
    bool started() const { return started_; }

    // 获取名字
    const std::string name() const { return name_; }

private:
    EventLoop *baseLoop_;                                   // 用户使用muudo创建的loop，若线程数为1 则直接使用用户创建的loop，否则创建多EventLoop
    std::string name_;                                      // 线程池名称，通常由用户指定，线程池中EventLoopThread名称依赖于线程池名称
    bool started_;                                          // 是否已经启动
    int numThreads_;                                        // 线程数量
    int next_;                                              // 新连接到来，选择的EventLoop索引
    std::vector<std::unique_ptr<EventLoopThread>> threads_; // IO线程列表
    std::vector<EventLoop *> loops_;                        // 线程池EventLoop列表和EventLoopThread一一对应
};