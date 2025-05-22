#pragma once

/*
    noncopyable被继承以后，派生类对象可以正常构造和析构
    但无法拷贝构造和赋值
    同时其他类无法实例化noncopyable
*/
class noncopyable
{
 public: // 禁止拷贝构造和赋值
  noncopyable(const noncopyable&) = delete;
  void operator=(const noncopyable&) = delete;

 protected: // 其他类无法实例化noncopyable，只有派生类可以
  noncopyable() = default;
  ~noncopyable() = default;
};

