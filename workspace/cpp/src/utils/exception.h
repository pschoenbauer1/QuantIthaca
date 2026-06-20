#pragma once

#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
struct RuntimeErrorBuilderHelper
{
};

class RuntimeErrorBuilder
{
    std::string message_;

public:
    RuntimeErrorBuilder(const char* file, int line)
    {
        std::ostringstream oss;
        oss << file << ":" << line << ": ";
        message_ = oss.str();
    }

    template <typename T>
    RuntimeErrorBuilder&& operator<<(T&& value) &&
    {
        std::ostringstream oss;
        oss << message_ << std::forward<T>(value);
        message_ = oss.str();
        return std::move(*this);
    }

    // Optional: support stream manipulators like std::endl
    RuntimeErrorBuilder&& operator<<(std::ostream& (*manip)(std::ostream&)) &&
    {
        std::ostringstream oss;
        oss << message_;
        manip(oss);
        message_ = oss.str();
        return std::move(*this);
    }

    // Conversion to std::runtime_error
    operator std::runtime_error() const
    {
        if (message_.empty())
        {
            return std::runtime_error("Exception");
        }
        return std::runtime_error(message_);
    }
};

void operator|(const RuntimeErrorBuilderHelper&, const RuntimeErrorBuilder& builder)
{
    throw static_cast<std::runtime_error>(builder);
}

}  // namespace

// clang-format off

#define THROW RuntimeErrorBuilderHelper{} | RuntimeErrorBuilder(__FILE__, __LINE__)
#define PS_EXPECT_THAT(condition) if(!(condition)) THROW << "Condition " #condition << " is violated. "


template <typename T>
concept ComparableToFloat = requires(T t)
{
    t < 0.0;
    t > 0.0;
    t <= 0.0;
    t >= 0.0;
};

template <ComparableToFloat T>
const T PS_VALIDATE_POSITIVE(const T& x)
{
    PS_EXPECT_THAT(x>0) << "Validation 'Positive' failed for x = " << x; 
    return x;
}