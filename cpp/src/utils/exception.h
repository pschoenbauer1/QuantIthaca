#pragma once

#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
struct RuntimeErrorBuilderHelper {};

class RuntimeErrorBuilder {
   public:
    RuntimeErrorBuilder() = default;

    template <typename T>
    RuntimeErrorBuilder&& operator<<(T&& value) && {
        std::ostringstream oss;
        oss << message_ << std::forward<T>(value);
        message_ = oss.str();
        return std::move(*this);
    }

    // Optional: support stream manipulators like std::endl
    RuntimeErrorBuilder&& operator<<(std::ostream& (*manip)(std::ostream&)) && {
        std::ostringstream oss;
        oss << message_;
        manip(oss);
        message_ = oss.str();
        return std::move(*this);
    }

    // Conversion to std::runtime_error
    operator std::runtime_error() const {
        if (message_.empty()) {
            return std::runtime_error("Exception");
        }
        return std::runtime_error(message_);
    }

   private:
    std::string message_;
};

void operator|(const RuntimeErrorBuilderHelper&, const RuntimeErrorBuilder& builder) {
    throw builder;
}

}  // namespace

// clang-format off

#define THROW RuntimeErrorBuilderHelper{} | RuntimeErrorBuilder {}
#define PS_EXPECT_THAT(condition) if(!(condition)) THROW 