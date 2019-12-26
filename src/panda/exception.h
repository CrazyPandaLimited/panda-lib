#pragma once
#include <exception>
#include <vector>
#include "string.h"
#include "refcnt.h"

namespace panda {

struct stackframe {
    string file;
    string library;
    string name;
    string mangled_name;
    std::uint32_t line_no = 0;
    std::uint64_t address = 0;
    std::uint64_t offset = 0;

};

struct backtrace_info : Refcnt {
    virtual const std::vector<stackframe>& get_frames() const = 0;
    virtual string to_string() const = 0;
};

using raw_trace = std::vector<void*>;
using backtrace_producer = iptr<backtrace_info>(*)(const raw_trace&);

struct backtrace {
    static const constexpr int max_depth = 50;

    backtrace () noexcept;
    backtrace (const backtrace &other) noexcept;
    virtual ~backtrace ();
    backtrace& operator=(const backtrace& other) = default;

    iptr<backtrace_info> get_backtrace_info() const noexcept;
    const raw_trace& get_trace () const noexcept { return buffer; }

    static void install_producer(backtrace_producer& producer);

private:
    std::vector<void*> buffer;
};

template <typename T>
struct bt : T, backtrace {
    template<typename ...Args>
    bt (Args&&... args) noexcept : T(std::forward<Args...>(args...)) {}
};

struct exception : std::exception, backtrace {
    exception () noexcept;
    exception (const string& whats) noexcept;
    exception (const exception& oth) noexcept;
    exception& operator= (const exception& oth) noexcept;

    const char* what () const noexcept override;

    virtual string whats () const noexcept;

private:
    mutable string _whats;
};


}
