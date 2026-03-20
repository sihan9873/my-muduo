#pragma once
// 编译器级别防止头文件被重复包含

/*
    noncopyable被继承以后,派生类对象可以正常构造和析构,
    但派生类对象无法进行拷贝构造和赋值操作
*/
class noncopyable
{
public:
    // 删除拷贝构造和赋值重载
    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete;

protected:
    // 派生类构造要调用基类的构造,所以还是手写一下
    noncopyable() = default;
    ~noncopyable() = default;
};
