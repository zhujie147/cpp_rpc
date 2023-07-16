/*
1.日志级别 DEBUG INFO ERROR
2.打印到文件，支持日期命名，以及日志的滚动
    文件名、行号、进程号、日期、时间 自定义消息
    [level][%y-%m-%d %H:%H:%s.%ms]\t[pid:thread_id]\t[file_name:line][%msg]
3.c 格式化风格
4.线程安全

Logger 日志器
1.提供打印日志的方法
2.设置日志输出的路径
*/

#ifndef ROCKET_COMMON_LOG_H
#define ROCKET_COMMON_LOG_H
#include <string>

#include <queue>
#include <memory>
#include "rocket/common/config.h"
#include "rocket/common/mutex.h"

namespace rocket
{

    template <typename... Args>
    std::string formatString(const char *str, Args &&...args)
    {
        int size = snprintf(nullptr, 0, str, args...);
        std::string result;
        if (size > 0)
        {
            result.resize(size);
            snprintf(&result[0], size + 1, str, args...);
        }
        return result;
    }

#define DEBUGLOG(str, ...)                                                                                                                                         \
    if (rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Debug)                                                                                         \
    {                                                                                                                                                              \
        rocket::Logger::GetGlobalLogger()->pushLog((new rocket::LogEvent(rocket::LogLevel::Debug))->toString() +"["+std::string(__FILE__)+":"+std::to_string(__LINE__)+"]\t"+ rocket::formatString(str, ##__VA_ARGS__) + "\n"); \
        rocket::Logger::GetGlobalLogger()->log();                                                                                                                  \
    }

#define INFOLOG(str, ...)                                                                                                            \
    if (rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Info)                                                                                         \
    {                                                                                                                                                              \
        rocket::Logger::GetGlobalLogger()->pushLog((new rocket::LogEvent(rocket::LogLevel::Info))->toString() +"["+std::string(__FILE__)+":"+std::to_string(__LINE__)+"]\t"+ rocket::formatString(str, ##__VA_ARGS__) + "\n"); \
        rocket::Logger::GetGlobalLogger()->log();                                                                                                                  \
    }

#define ERRORLOG(str, ...)                                                                                                                                         \
    if (rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Error)                                                                                         \
    {                                                                                                                                                              \
        rocket::Logger::GetGlobalLogger()->pushLog((new rocket::LogEvent(rocket::LogLevel::Error))->toString() +"["+std::string(__FILE__)+":"+std::to_string(__LINE__)+"]\t"+ rocket::formatString(str, ##__VA_ARGS__) + "\n"); \
        rocket::Logger::GetGlobalLogger()->log();                                                                                                                  \
    }

    enum LogLevel
    {
        Unknown = 0,
        Debug = 1,
        Info = 2,
        Error = 3,
    };
    std::string LogLevelToString(LogLevel level);
    LogLevel StringToLogLevel(const std::string &log_level);

    class Logger
    {
    public:
        typedef std::shared_ptr<Logger> s_ptr;
        Logger(LogLevel level) : m_set_level(level) {}
        void pushLog(const std::string &msg);

        void log(); // 打印log

        LogLevel getLogLevel() const
        {
            return m_set_level;
        }

    public:
        static Logger *GetGlobalLogger();
        static void InitGlobalLogger();

    private:
        LogLevel m_set_level;
        std::queue<std::string> m_buffer;

        Mutex m_mutex;
    };

    class LogEvent
    {
    public:
        LogEvent(LogLevel level) : m_level(level) {}
        std::string getFileName() const
        { // 后面加const表示不可以修改class的成员
            return m_file_name;
        }

        LogLevel getLogLevel() const
        {
            return m_level;
        }

        std::string toString();

    private:
        std::string m_file_name;
        int32_t m_file_line;
        int32_t m_pid;
        int32_t m_thread_id;

        LogLevel m_level;
    };
}

#endif