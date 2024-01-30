#pragma once

#include <string>
#include <fstream>
#include <cstdio>

#include <variant>

#include "macros.h"
#include "time.h"
#include "thread_utils.hpp"
#include "lf_queue.hpp"
#include "time_utils.hpp"

namespace common 
{
    constexpr size_t LOG_QUEUE_SIZE = 8 * 1024 * 1024;

    enum class LogType : int8_t
    {
        CHAR = 0, 
        INTEGER = 1,
        LONG_INTEGER = 2,
        LONG_LONG_INTEGER = 3,
        UNSIGNED_INTEGER = 4,
        UNSIGNED_LONG_INTEGER = 5,
        UNSIGNED_LONG_LONG_INTEGER = 6,
        FLOAT = 7,
        DOUBLE = 8
    };

    struct LogElement
    {
        LogType type_ = LogType::CHAR;
        
        union 
        {
            char c;
            int i;
            long l;
            long long ll;
            unsigned u;
            unsigned long ul;
            unsigned long long ull;
            float f;
            double d;
        } u_;
        
        //type-safe union in modern C++, but not used because of worse performance runtime
        /* 
        std::variant<char, int, long, long long, unsigned, unsigned long, unsigned long long, float, double> 
        c, i, l, ll, u, ul, ull, f, d;
        */
    };

    class Logger final 
    {
    public:
        auto flushQueue() noexcept 
        {
            while (running_)
            {
                for (auto next = queue_.getNextToRead(); queue_.size() && next; next = queue_.getNextToRead())
                {
                    switch (next->type_)
                    {
                        case LogType::CHAR:
                            file_ << next->u_.c;
                            break;
                        case LogType::INTEGER:
                            file_ << next->u_.i;
                            break;
                        case LogType::LONG_INTEGER:
                            file_ << next->u_.l;
                            break;
                        case LogType::LONG_LONG_INTEGER:
                            file_ << next->u_.ll;
                            break;
                        case LogType::UNSIGNED_INTEGER:
                            file_ << next->u_.u;
                            break;
                        case LogType::UNSIGNED_LONG_INTEGER:
                            file_ << next->u_.ul;
                            break;
                        case LogType::UNSIGNED_LONG_LONG_INTEGER:
                            file_ << next->u_.ull;
                            break;
                        case LogType::FLOAT:
                            file_ << next->u_.f;
                            break;
                        case LogType::DOUBLE:
                            file_ << next->u_.d;
                            break;
                        default:
                            break;
                    }
                    queue_.updateReadIndex();
                    next = queue_.getNextToRead();
                }
                using namespace std::literals::chrono_literals;
                std::this_thread::sleep_for(1ms);
            }
        }

        explicit Logger(const std::string &file_name) : file_name_(file_name), queue_(LOG_QUEUE_SIZE)
        {
            file_.open(file_name);
            ASSERT(file_.is_open(), "Could not open log file: " + file_name);
            logger_thread_ = createAndStartThread(-1, "common/Logger", [this]()
                                                  { flushQueue(); });

            ASSERT(logger_thread_ != nullptr, "Failed to start Logger thread.");
        }

        ~Logger() 
        {
            std::cerr << "Flusing and closing Logger for " << file_name_ << std::endl;

            //wait until queue_ is no longer in use;
            while (queue_.size())
            {
                using namespace std::literals::chrono_literals;
                std::this_thread::sleep_for(1s);
            }

            //set running_ flag to false to prevent it from running again
            running_ = false;
            //end logger_thread_
            logger_thread_->join();
            //deconstruct filestream handle
            file_.close();
        }

        Logger() = delete;
        Logger(const Logger &) = delete;
        Logger(const Logger &&) = delete;
        Logger &operator=(const Logger &) = delete;
        Logger &operator=(const Logger &&) = delete;

        
        auto pushValue(const LogElement &log_element) noexcept
        {
            *(queue_.getNextToWriteTo()) = log_element;
            queue_.updateWriteIndex();
        }

        //push a single character
        auto pushValue(const char value) noexcept
        {
            pushValue(LogElement{LogType::CHAR, {.c = value}});
        }

        //push a collection of character
        auto pushValue(const char *value) noexcept
        {
            while (*value) {
                pushValue(*value);
                value++;
            }
        }

        //push an std::string object
        auto pushValue(const std::string &value) noexcept
        {
            pushValue(value.c_str());
        }

        auto pushValue(const int value) noexcept
        {
            pushValue(LogElement{LogType::INTEGER, {.i = value}});
        }

        auto pushValue(const long value) noexcept
        {
            pushValue(LogElement{LogType::LONG_INTEGER, {.l = value}});
        }

        auto pushValue(const long long value) noexcept
        {
            pushValue(LogElement{LogType::LONG_LONG_INTEGER, {.ll = value}});
        }

        auto pushValue(const unsigned value) noexcept
        {
            pushValue(LogElement{LogType::UNSIGNED_INTEGER, {.u = value}});
        }

        auto pushValue(const unsigned long value) noexcept
        {
            pushValue(LogElement{LogType::UNSIGNED_LONG_INTEGER, {.ul = value}});
        }

        auto pushValue(const unsigned long long value) noexcept
        {
            pushValue(LogElement{LogType::UNSIGNED_LONG_LONG_INTEGER, {.ull = value}});
        }

        auto pushValue(const float value) noexcept
        {
            pushValue(LogElement{LogType::FLOAT, {.f = value}});
        }

        auto pushValue(const double value) noexcept
        {
            pushValue(LogElement{LogType::DOUBLE, {.d = value}});
        }

        template <typename T, typename ... A>
        auto log (const char *s, const T &value, A... args) noexcept
        {
            while (*s)
            {
                if (*s == '%')
                {
                    if (UNLIKELY(*(s+1) == '%'))
                    {
                        s++;
                    } else 
                    {
                        pushValue(value);
                        log (s + 1, args...);
                        return;
                    }
                }
                pushValue(*s++);
            }
            FATAL("Extra arguments provided to log()");
        }

        auto log (const char *s) noexcept
        {
            while (*s)
            {
                if (*s == '%')
                {
                    if (UNLIKELY( *(s + 1) == '%'))
                    {
                        s++;
                    } else 
                    {
                        FATAL("Missing arguments to log()");
                    }
                }
                pushValue(*s++);
            }
        }

    private:
        const std::string file_name_;
        std::ofstream file_;
        common::LFQueue<LogElement> queue_;
        std::atomic<bool> running_ = {true};
        std::thread *logger_thread_ = nullptr;
    };
}