---
created: "2025/05/30 14:50"
updated: "2025/06/04 19:43"
---

留档

```mermaid
sequenceDiagram
    participant Event
    participant Reactor
    participant Demultiplex
    participant EventHandler

    Event->>Reactor: 注册Event和Handler
    loop 事件集合
        Reactor->>Reactor: Event集合
    end
    Reactor->>Demultiplex: 向Epoll add/mod/del Event
    Reactor->>Demultiplex: 启动反应堆
    loop 事件分发器
        Demultiplex->>Demultiplex: 开启事件循环epoll wait
    end
    Demultiplex->>Reactor: 返回发生事件的Event
    Reactor->>EventHandler: 调用Event对应的事件处理器EventHandler
```

![](./assets/%E8%B7%9F%E9%9A%8F%E6%96%BD%E7%A3%8A%E8%80%81%E5%B8%88%E8%A7%86%E9%A2%91%E6%89%8B%E6%95%B2muduo-media/ab104c58d5d80f963bc7bb6c972e8002dca18da9.jpeg)

图中每个subReactor都是一个EventLoop

![](./assets/%E8%B7%9F%E9%9A%8F%E6%96%BD%E7%A3%8A%E8%80%81%E5%B8%88%E8%A7%86%E9%A2%91%E6%89%8B%E6%95%B2muduo-media/01927a856c644557b3e28245df6b91225e7e0200.svg)

![](./assets/%E8%B7%9F%E9%9A%8F%E6%96%BD%E7%A3%8A%E8%80%81%E5%B8%88%E8%A7%86%E9%A2%91%E6%89%8B%E6%95%B2muduo-media/7501ec223b68c2e68ecb1d97f61fee60e71be683.jpeg)

    新连接到来，有的会被分配到mainLoop,有的分配到subLoop
    分发线程是main线程，如果正好分配轮询到本线程（acceptor所在线程），则可以直接执行，如果轮询到其他线程，则需要唤醒其线程

    始终记住，线程的数据是共享的

![](./assets/%E8%B7%9F%E9%9A%8F%E6%96%BD%E7%A3%8A%E8%80%81%E5%B8%88%E8%A7%86%E9%A2%91%E6%89%8B%E6%95%B2muduo-media/d38638118204908d0e7a061bef2621657a9618d8.jpeg)

![](./assets/%E8%B7%9F%E9%9A%8F%E6%96%BD%E7%A3%8A%E8%80%81%E5%B8%88%E8%A7%86%E9%A2%91%E6%89%8B%E6%95%B2muduo-media/998fa36efeb14221cc7ac5b8d26fd2abfe856553.jpeg)

![](./assets/%E8%B7%9F%E9%9A%8F%E6%96%BD%E7%A3%8A%E8%80%81%E5%B8%88%E8%A7%86%E9%A2%91%E6%89%8B%E6%95%B2muduo-media/52ad007b4980ad24c518046d9837f9aef10e9ef6.png)

## 宏的高级用法

``` cpp
// LOGINFO("%s %d", arg1, arg2)
#define LOG_INFO(logmsgFormat, ...) \
    do \
    { \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, sizeof(buf), logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)
```

- 定义一个宏 `LOG_INFO`，用于记录 **INFO 级别** 的日志。
- 格式化日志消息并调用 `Logger` 类的 `log()` 方法输出。

#### `do { ... } while(0)`

``` cpp
do \
{ \
    // ... \
} while(0)
```

- 确保宏在语法上像一个 **独立语句**，避免在使用时因分号或作用域问题导致编译错误。

- 例如：

  ``` cpp
  if (condition)
      LOG_INFO("Hello");  // 如果没有 do-while(0)，可能会解析错误
  else
      ...
  ```

  如果没有 `do-while(0)`，`else` 可能会匹配到宏内部的 `if`（如果宏内部有 `if`），导致编译错误。

#### `##__VA_ARGS__`

- 用于处理可变参数`(...)`。
- 如果 LOG_INFO 后面没有参数（如 `LOG_INFO("Hello")`），`##`会去掉多余的逗号，避免编译错误：

#### 反斜杠 `\`

在 C/C++ 宏定义中，**反斜杠 `\`（续行符）** 的作用是 **让宏定义跨越多行**，使其在语法上看起来像一行代码。

## TcpServer

![](./assets/%E8%B7%9F%E9%9A%8F%E6%96%BD%E7%A3%8A%E8%80%81%E5%B8%88%E8%A7%86%E9%A2%91%E6%89%8B%E6%95%B2muduo-media/6f5bc78a02d3d9e2e42172270e585a9f6bccc3ad.jpeg)
