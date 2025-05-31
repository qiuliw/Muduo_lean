#pragma  once

#include <sys/syscall.h>
#include <unistd.h>

namespace CurentThreead 
{
    //线程 唯一的变量
    extern __thread int t_cachedTid;
    // 避免内核态获取tid的损耗
    void cacheTid();
    
    inline int tid(){
        if(__builtin_expect(t_cachedTid == 0,0)){
            cacheTid();
        }
        return t_cachedTid;
    }
}