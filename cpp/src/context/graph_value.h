#pragma once

#include <concepts>
#include <string>

#include "pointers.h"
#include "threads.h"

namespace graph {

class GraphValue {
   public:
    virtual ~GraphValue() = default;
};

class DummyValue1 : public GraphValue {
    double _x;

   public:
    explicit DummyValue1(double x) : _x(x) {}

    double get_x() const { return _x; }

    static std::string name() { return "DummyValue1"; }
};
class DummyValue2 : public GraphValue {
    double _y;

   public:
    explicit DummyValue2(double y) : _y(y) {}

    double get_y() const { return _y; }

    static std::string name() { return "DummyValue2"; }
};

enum class ValueState { initialized, invalidated, exception, value, deferred };

class ValueWrapper {
    std::variant<std::monostate, CPtr<GraphValue>, std::exception_ptr> _data{};
    mutable utils::Mutex _mutex;

    // --- state checks ---
    bool is_empty() const {
        utils::ReadLock lock(_mutex);
        return std::holds_alternative<std::monostate>(_data);
    }

    bool has_value() const {
        utils::ReadLock lock(_mutex);
        return std::holds_alternative<CPtr<GraphValue>>(_data);
    }

    bool has_error() const {
        utils::ReadLock lock(_mutex);
        return std::holds_alternative<std::exception_ptr>(_data);
    }

   public:
    ValueWrapper() = default;
    ValueWrapper(const CPtr<GraphValue>& value) { set_value(value); }
    ValueWrapper(std::exception_ptr error) { set_error(error); }

    // --- accessors ---
    CPtr<GraphValue> value() const {
        rethrow_if_error();
        throw_if_empty();

        utils::ReadLock lock(_mutex);
        return std::get<CPtr<GraphValue>>(_data);
    }

    std::exception_ptr error() const { return std::get<std::exception_ptr>(_data); }

    void rethrow_if_error() const {
        if (has_error()) {
            utils::ReadLock lock(_mutex);
            std::rethrow_exception(std::get<std::exception_ptr>(_data));
        }
    }
    void throw_if_empty() const {
        if (is_empty()) {
            THROW << "Value is empty";
        }
    }

    void set_error(std::exception_ptr exc) {
        if (!is_empty()) {
            THROW << "Attempted to set an error on a non-empty value wrapper.";
        }
        utils::WriteLock lock(_mutex);
        _data = exc;
    }
    void set_value(CPtr<GraphValue> value) {
        if (!is_empty()) {
            THROW << "Attempted to set a value on a non-empty value wrapper.";
        }
        utils::WriteLock lock(_mutex);
        _data = std::move(value);
    }
};
}  // namespace graph