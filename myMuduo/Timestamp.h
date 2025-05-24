// Timestamp 时间戳

#pragma once

#include <iostream>
#include <string.h>

class Timestamp{
public:
    Timestamp();
    // 带参构造使用explicit避免隐式转换
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp now();
    std::string toString() const;

private:
    int64_t microSecondsSinceEpoch_;
};