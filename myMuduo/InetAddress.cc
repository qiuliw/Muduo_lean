#include "InetAddress.h"

#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <ostream>
#include <strings.h>
#include <sys/socket.h>

// 默认值声明和定义中只能出现一次
InetAddress::InetAddress(uint16_t port,std::string ip){
    bzero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

std::string InetAddress::toIp() const{
    // addr_
    char buf[64] = {0};
    // 将网络字节序的二进制 IP 地址转换为人类可读的字符串格式的函数
    ::inet_ntop(AF_INET,&addr_.sin_addr, buf,sizeof buf);
    return buf;
}

std::string InetAddress::toIpPort() const{
    // ip:port
    char buf[64] = {0};
    ::inet_ntop(AF_INET,&addr_.sin_addr, buf,sizeof buf);
    size_t end = strlen(buf);

    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf+end, ":%u", port);
    return buf;
}

uint16_t InetAddress::toPort() const{
    return ntohs(addr_.sin_port);
}

#include <iostream>
int main(){

    InetAddress addr(8080);

    std::cout << addr.toIpPort() << std::endl;
    

    return 0;
}