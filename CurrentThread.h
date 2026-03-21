#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    // 一个全局变量,原本是进程下所有线程共享的
    // 加上__thread / local_thread的修饰,则表示
    // 每个线程内部各自拷贝一份,在自己线程的修改,别的线程看不到
    __thread int t_cacheTid = 0;

    // 因为获取tid需要系统调用
    // 得到之后保存到本地,减少从用户态到内核态的切换开销
    void cacheTid();

    inline int tid()
    {
        if (__builtin_expect(t_cacheTid == 0, 0))
        {
            cacheTid();
        }
        return t_cacheTid;
    }
}
