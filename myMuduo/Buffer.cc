#include "Buffer.h"

#include <sys/uio.h>

// 从流式管道fd读取数据
// 从fd中读取数据，Poller工作在LT模式下
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[65536] = {0}; // 栈上的内存空间 64K
    struct iovec vec[2];
    
    const size_t writable = writableBytes(); // 这是Buffer底层缓冲区剩余的可写空间大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovent = (writable < sizeof extrabuf) ? 2 : 1;

    const ssize_t n = ::readv(fd, vec, iovent);

    if(n< 0){
        *saveErrno = errno;
    }else if( n<=writable)
    {
        writerIndex_ += n;
    }else{ //extraBuf也写入了数据
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }

    return n;

}