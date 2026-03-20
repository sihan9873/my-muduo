#pragma once

#include <string>
#include "noncopyable.h"

/*
定义日志的级别
INFO 打印重要的流程信息,正常的日志输出,跟踪核心流程
ERROR 错误,但不影响赤瓜帅继续执行
FATAL 重大错误,程序不发正常运行,exit
DEBUG 调试信息
写成宏以便于用户调用
*/
// LOG_INFO("%s %s",arg1,arg2)
#define LOG_INFO(logmsgFormat, ...)          \
    do                                       \
    {                                        \
        logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO);            \
        char buf[1024] = {0};                \
        logger.log(buf);                     \
    } while (0)

#define LOG_ERROR(logmsgFormat, ...)         \
    do                                       \
    {                                        \
        logger &logger = Logger::instance(); \
        logger.setLogLevel(ERROR);           \
        char buf[1024] = {0};                \
        logger.log(buf);

#define LOG_FATAL(logmsgFormat, ...)         \
    do                                       \
    {                                        \
        logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL);           \
        char buf[1024] = {0};                \
        logger.log(buf);

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...)         \
    do                                       \
    {                                        \
        logger &logger = Logger::instance(); \
        logger.setLogLevel(DEBUG);           \
        char buf[1024] = {0};                \
        logger.log(buf);
#else
#define LOG_DEBUG(logmsgFormat, ...)
#endif

enum LogLevel
{
    INFO,  // 普通信息
    ERROR, // 错误信息
    FATAL, // core信息
    DEBUG, // 调试信息
};

// 单例(Singleton),C++ 中最常用的设计模式之一
// 核心目标是保证一个类在程序运行期间只有一个实例，并提供全局唯一的访问入口
// 这个全局的、统一的方法（通常是 getInstance()）用来获取这个唯一实例，无需像普通类那样通过 new 创建。

// 输出一个日志类
class Logger
{
public:
    // 获取日志唯一的实例对象
    static Logger &instance();
    // 设置日志级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);

private:
    int logLevel_;
    Logger() {}
};