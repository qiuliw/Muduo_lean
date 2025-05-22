#include <iostream>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include <boost/bind.hpp>
#include <muduo/net/EventLoop.h>

int main(int, char**){
    std::cout << "Hello, from muduo!\n";
}
