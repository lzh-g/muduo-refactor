#pragma once

#include <algorithm>
#include <stddef.h>
#include <string>
#include <vector>

/**
 * @brief 缓冲区类
 */
class Buffer
{
public:
    static const size_t kCheapPrepend = 8; // 初始预留的prependable空间大小，用于记录数据长度
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize) : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend) {}

    // 可读区的数据大小
    size_t readableBytes() const { return writerIndex_ - readerIndex_; }

    // 可写区的数据大小
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }

    // 预留空间大小 = 初始预留空间大小(8) + 已读的可读取大小 = readerIndex_的值
    size_t prependableBytes() const { return readerIndex_; }

    // 返回缓冲区中可读数据的起始地址
    const char *peek() const { return begin() + readerIndex_; }

    // 读取len长度的空间
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            // 读取len长度数据，可读区起始指针+len
            readerIndex_ += len;
        }
        else
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    // 将可读区数据读取为string
    std::string retrieveAllAsString()
    {
        return retrieveAllAsString(readableBytes());
    }
    std::string retrieveAllAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len); // 读取数据后，可读区起始指针相应移动
        return result;
    }

    // 需要len长度的可写区
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len); // 扩容
        }
    }

    // 将[data, data + len]内存上的数据添加到可读区
    void append(const char *data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }
    char *beginWrite() { return begin() + writerIndex_; }
    const char *beginWrite() const { return begin() + writerIndex_; }

    // 从fd上读数据
    ssize_t readFd(int fd, int *saveErrno);

    // 通过fd发数据
    ssize_t writeFd(int fd, int *saveErrno);

private:
    // vector底层数组起始地址
    char *begin() { return &*buffer_.begin(); }
    const char *begin() const { return &*buffer_.begin(); }

    /**
     * @brief 腾出len长度的空闲空间
     * | kCheapPrepend | 已读空闲1 | 未读 | 可写空闲2 |
     * | kCheapPrepend | 未读 | 已读空闲1 + 可写空闲2 |
     */
    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            // 当前可写区域不足以写入当前的内容，扩容
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            // 将未读区与KCheapPrepend合并，使得已读空闲1 + 可写空闲2合并为大的空闲空间
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_; // 可读区的起始位置
    size_t writerIndex_; // 空闲区的起始位置
};