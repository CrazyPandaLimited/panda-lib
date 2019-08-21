#include "exception.h"

#if !defined(__unix__)
namespace panda {
    backtrace::backtrace noexcept {}
    string backtrace::get_trace_string () const { return {}; }
}
#else

#include <execinfo.h>
#include <cstring>
#include <memory>
#include <functional>
#include <regex>
#include <cxxabi.h>

namespace panda {

backtrace::backtrace () noexcept {
    buffer.resize(max_depth);
    auto depth = ::backtrace(buffer.data(), max_depth);
    buffer.resize(depth);
}

backtrace::backtrace (const backtrace &other) noexcept : buffer(other.buffer) {}

static panda::string humanize (const char* symbol) {
    std::regex re("(.+)\\((.+)\\+0x(.+)\\) \\[(.+)\\]");
    std::cmatch what;
    if (regex_match(symbol, what, re)) {
        panda::string dll           (what[1].first, what[1].length());
        panda::string mangled_name  (what[2].first, what[2].length());
        panda::string symbol_offset (what[3].first, what[3].length());
        panda::string address       (what[4].first, what[4].length());

        int status;
        char* demangled_name = abi::__cxa_demangle(mangled_name.c_str(), nullptr, 0, &status);
        if (demangled_name) {
            using guard_t = std::unique_ptr<char*, std::function<void(char**)>>;
            guard_t guard(&demangled_name, [](char** ptr) { free(*ptr); });
            // mimic gdb style, i.e.
            // 0x00007ffff77c832c in Catch::TestInvokerAsFunction::invoke() const () from ../../var/lib/x86_64-linux/auto/Test/Catch/Catch.so
            return address + " in " + demangled_name + " from " + dll;
        }
    }
    return panda::string("[demangle failed]") + symbol;
}

string backtrace::get_trace_string () const {
    panda::string result = "";
    using guard_t = std::unique_ptr<char**, std::function<void(char***)>>;
    char** symbols = backtrace_symbols(buffer.data(), buffer.size());
    if (symbols) {
        guard_t guard(&symbols, [](char*** ptr) { free(*ptr); });
        for (int i = 0; i < static_cast<int>(buffer.size()); ++i) {
            result += humanize(symbols[i]) + "\n";
        }
    }
    return result;
}


exception::exception () noexcept {}

exception::exception (const exception& oth) noexcept : std::exception(oth), backtrace(oth) {}

exception& exception::operator= (const exception& oth) noexcept {
    std::exception::operator=(oth);
    backtrace::operator=(oth);
    return *this;
}

}

#endif
