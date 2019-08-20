#include "backtrace.h"

#if defined(__linux__)

#include <execinfo.h>
#include <cstring>
#include <memory>
#include <functional>
#include <regex>
#include <cxxabi.h>

#endif

namespace panda {

backtrace_base::backtrace_base() noexcept {
#if defined(__linux__)
    buffer.resize(max_depth);
    auto depth = ::backtrace(buffer.data(), max_depth);
    buffer.resize(depth);
#endif
}


backtrace_base::backtrace_base(const backtrace_base &other) noexcept: buffer(other.buffer) {}


#if defined(__linux__)
static panda::string humanize(const char* symbol) {
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
#endif

string backtrace_base::get_trace_string() const {
    panda::string result = "";
#if defined(__linux__)
    using guard_t = std::unique_ptr<char**, std::function<void(char***)>>;
    char** symbols = backtrace_symbols(buffer.data(), buffer.size());
    if (symbols) {
        guard_t guard(&symbols, [](char*** ptr) { free(*ptr); });
        for (int i = 0; i < static_cast<int>(buffer.size()); ++i) {
            result += humanize(symbols[i]) + "\n";
        }
    }
#endif
    return result;
}


}
