#pragma once

#include <algorithm>
#include <cstddef>
#include <sys/types.h>
#include <vector>
#include <string>

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer

/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  | writable bytes   |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writeIndex    <=     size
/// @endcode

// 为什么将可写区域放在末尾：方便vector数组扩容以及数据添加


// 网络库底层的缓冲区类型定义
class Buffer
{
public:

    // 在网络编程中，很多协议需要在数据包的头部添加额外信息，例如消息长度、时间戳、协议版本等。如果缓冲区起始位置没有预留空间，当需要添加这些信息时，就需要将整个数据向后移动，这会带来额外的性能开销。
    // 通过预先分配 kCheapPrepend 大小的空间，可以直接在这个区域写入头部信息，而不需要移动原有数据。这样做既高效又简洁。


    // 定义缓冲区使用的起始位置
    static const size_t kCheapPrepend = 8;
    //  缓冲区初始大小
    static const size_t kInitalSize = 1024;

    explicit Buffer(size_t initialSize = kInitalSize)
        :buffer_(kCheapPrepend + initialSize)
        ,readerIndex_(kCheapPrepend)
        ,writerIndex_(kCheapPrepend)
    {
        // 预留空间
        buffer_.resize(kInitalSize);
    }

    // 获取缓冲区可读数据的长度
    size_t readableBytes() const{
        return writerIndex_ - readerIndex_;
    }
    // 获取缓冲区末尾可写数据的长度
    size_t writableBytes() const{
        return buffer_.size() - writerIndex_;
    }
    //  readerIndex_ 返回头部可用空间的长度，包含 kCheapPrepend
    size_t prependableBytes() const{
        return readerIndex_;
    }

    // 返回缓冲区可读数据的起始位置
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    // onMessage string <- Buffer
    void retrieve(size_t len){ 

        if(len < readableBytes())
        {    
            readerIndex_ += len; // 应用只读取了可读缓冲区数据的一部分，就是len，还剩下 readerIndex-writerIndex
        }
        else{ // len >= readableBytes()
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        // 上面一句把缓冲区中的数据读出，
        // 所以这里要对缓冲区进行复位操作
        retrieve(len); 
        return result;
    }


    // 把onMessage函数上报的Buffer数据，封装成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    // buffer_.size - writeIndex_ < len 需要扩容
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len); //  扩容或移动数据 腾出空间
        }
    }

    // 添加数据
    void append(const char *data,size_t len)
    {
        ensureWritableBytes(len);
        // data ~ data+len ==> begin()+writerIndex_
        std::copy(data, data + len, begin() + writerIndex_);
        writerIndex_ += len;
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const{
        return begin() + writerIndex_;
    }

    //  从fd上读取数据
    ssize_t readFd(int fd, int *saveErrno);
private:
    // 缓冲区起始位置
    char* begin() {
        // it.operator*()
        // return &*buffer_.begin();
        return buffer_.data();
    }

    // 不希望改变缓冲区内容，所以返回const char*
    const char* begin() const{
        return &*buffer_.begin();
    }

    // 扩容函数
    void makeSpace(size_t len)
    {

        // Buffer没有封装为一个环，所以当Buffer使用一段时间后，
        // 其真正的可写入空间不一定是size() - writeIndex，
        // 因为readIndex和prependable之间可能还有空间。因此当可写如空间不够时，有时可以通过把CONTENT向“前移”来达到目的。当向前移还不能满足时，才真正开辟新空间。

        // 可写的字节数 + 前面空余的  < len +  kCheapPrepend
        if(writableBytes() + prependableBytes() < len + kCheapPrepend){
            buffer_.resize(writerIndex_ + len);
        }else{ // 空间足够，则将数据移动到缓冲区前面，重新排列数据
            size_t readable = readableBytes(); // 可读的字节数
            std::copy(begin() + readerIndex_, 
            begin() + writerIndex_, 
            begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }



    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};