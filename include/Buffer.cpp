#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/**
 * 从fd上读取数据 Poller工作在LT模式
 * 从fd上读取数据时，不知道tcp数据大小，buffer_空间可能不够
 * @brief 先使用readv读取数据至buffer_，若空间不够则使用extrabuf暂存数据，再以append方式追加buffer_空间，避免系统调用带来的开销，且不影响数据接收
 */
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[655336] = {0}; // 64KB额外栈空间

    struct iovec vec[2];
    const size_t writable = writableBytes();

    // 第一块缓冲区，指向可写空间
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    // 第二块缓冲区，指向栈空间
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);

    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable)
    {
        // buffer_可写缓冲区已经够读出来的数据
        writerIndex_ += n;
    }
    else
    {
        // extrabuf里也写入了n - writable长度的数据
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable); // 对buffer_扩容，并将extrabuf存储的另一部分数据追加到buffer_
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}
