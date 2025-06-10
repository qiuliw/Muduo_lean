#include <mymuduo/TcpServer.h>
#include <mymuduo/Logger.h>
#include <string>

class EchoServer{
public:
    EchoServer(EventLoop *loop,
            const InetAddress& listenAddr,
            const std::string& nameArg)
            :server_(loop, listenAddr, nameArg)
            , loop_(loop)
    {
        // 注册回调函数
        server_.setConnectionCallback([this](const TcpConnectionPtr &conn){
                onConnection(conn);
            }
        );

        server_.setMessageCallback([this](const TcpConnectionPtr &conn,
                                    Buffer* buf,
                                    Timestamp time){
                onMessage(conn, buf, time);
            }
        );

        // 设置合适的loop线程数量
        server_.setThreadNum(3);

    }

    void start()
    {
        server_.start();
    }

        
private:

    // 连接建立或断开的回调
    void onConnection(const TcpConnectionPtr &conn)
    {
        if(conn->connected()){
            //peerAddr() 返回 const InetAddress&，但宏内部调用的是 toIpPort()，它返回 std::string，最终传递给 snprintf 的是 const char*。
            LOG_INFO("conn UP : %s",conn->peerAddr().toIpPort().c_str());
        }
        else{
            LOG_INFO("conn DOWN : %s",conn->peerAddr().toIpPort().c_str());
        }

    }

    
    // 可读写事件回调
    void onMessage(const TcpConnectionPtr& conn,
            Buffer* buf,
            Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        LOG_INFO("recv from %s at %s\n",
                conn->peerAddr().toIpPort().c_str(),
                time.toString().c_str());
        conn->shutdown(); // 写端 EPOLLHUB => closeCallback_
    }

    EventLoop  *loop_;
    TcpServer server_;
};

int main(){

    EventLoop loop;
    InetAddress addr(8000);
    EchoServer server(&loop, addr, "EchoServer-01");// Acceptor non-blocking listeningfd
    server.start(); // listen loopthread ==> acceptChannel_ ==> mainLoop
    loop.loop(); // 启动 mainLoop底层loop

    return 0;
}