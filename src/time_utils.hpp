#pragma once

#include <chrono>
#include <ctime>

#include <string>

namespace common
{
    typedef int64_t nanos;
    
    constexpr nanos NANOS_TO_MICROS = 1000;
    constexpr nanos MICROS_TO_MILLIS = 1000;
    constexpr nanos MILLIS_TO_SECS = 1000;
    constexpr nanos NANO_TO_MILLIS = NANOS_TO_MICROS * MICROS_TO_MILLIS;
    constexpr nanos NANOS_TO_SECS = NANO_TO_MILLIS * MILLIS_TO_SECS;

    inline auto getCurrentNanos() noexcept 
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    inline auto &getCurrentTimeString(std::string *time_str) noexcept
    {
        const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        time_str->assign(ctime(&time));

        if (!time_str->empty())
        {
            time_str->at(time_str->length() - 1) = '\0';
        }

        return *time_str;
    }
}