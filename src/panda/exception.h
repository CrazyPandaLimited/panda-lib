#pragma once
#include <exception>
#include <vector>
#include "string.h"
#include "refcnt.h"

namespace panda {

struct Stackframe: public Refcnt {
    string file;
    string library;
    string name;
    string mangled_name;
    std::uint64_t address = 0;
    std::uint64_t offset = 0;
    std::uint64_t line_no = 0;
    std::vector<string> args;
};

using StackframePtr = iptr<Stackframe>;

struct BacktraceInfo : Refcnt {
    BacktraceInfo(std::vector<StackframePtr>&& frames_) : frames(std::move(frames_)) {}
    virtual ~BacktraceInfo();
    const std::vector<StackframePtr>& get_frames() const noexcept { return frames;}
    virtual string to_string() const noexcept;

    std::vector<StackframePtr> frames;
};

using RawTrace = std::vector<void*>;
using RawTraceProducer = int(*)(void**, int);
using BacktraceProducer = StackframePtr(*)(const void* raw_frame);

struct Backtrace {
    static const constexpr int max_depth = 50;

    Backtrace () noexcept;
    Backtrace (const Backtrace &other) noexcept;
    virtual ~Backtrace ();
    Backtrace& operator=(const Backtrace& other) = default;

    iptr<BacktraceInfo> get_backtrace_info() const noexcept;
    const RawTrace& get_trace () const noexcept { return buffer; }

    static void install_producer(BacktraceProducer& producer);
    static string dump_trace() noexcept;

private:
    std::vector<void*> buffer;
};

template <typename T>
struct bt : T, Backtrace {
    template<typename ...Args>
    bt (Args&&... args) noexcept : T(std::forward<Args...>(args...)) {}
};

struct exception : std::exception, Backtrace {
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
