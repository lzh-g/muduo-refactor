#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

/**
 * @brief 监听类，封装监听套接字及相关处理方法，运行在主线程mainloop，监听并接收新连接，接收到的连接分发给subloop
 */
class Acceptor : public noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;

    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();
    // 设置新连接的回调函数
    void setNewConnectionCallback(const NewConnectionCallback &cb) { newConnectionCallback_ = cb; }
    // 判断是否在监听
    bool listenning() const { return listenning_; }
    // 监听本地端口
    void listen();

private:
    // 处理新用户的连接事件
    void handleRead();

    EventLoop *loop_; // mainloop

    Socket acceptSocket_; // 用于接收新连接的socket

    Channel acceptChannel_; // 监听新连接的channel

    NewConnectionCallback newConnectionCallback_; // 新连接的回调

    bool listenning_; // 是否在监听
};