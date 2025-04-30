#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    extern __thread int t_cachedTid; // 保存当前线程的tid，以免频繁系统调用(耗时)

    void cacheTid();

    // 内联函数只在当前文件中起作用
    inline int tid()
    {
        // builtin_expect是一种底层优化，表示若还未获取tid，则调用cacheTid()获取tid
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
} // namespace CurrentThread