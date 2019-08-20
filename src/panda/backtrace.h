#pragma once

#include <exception>
#include <vector>
#include "string.h"

namespace panda {

class backtrace_base {
public:
    static const constexpr int max_depth = 50;
    backtrace_base() noexcept;
    backtrace_base(const backtrace_base &other) noexcept;

    string get_trace_string() const;
    const std::vector<void*>& get_trace() const noexcept { return buffer; }
private:
    std::vector<void*> buffer;
};

template <typename T>
class backtrace: public T, public backtrace_base {
public:
    template<typename ...Args>
    backtrace(Args&&... args) noexcept: T(std::forward<Args...>(args...)), backtrace_base() {}
};

}
