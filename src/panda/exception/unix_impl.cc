#include "unix_impl.h"

#if defined(__unix__) || defined(__APPLE__)
#include <execinfo.h>
#include <regex>
#include <functional>
#include <cxxabi.h>
#include <memory>

namespace panda { namespace impl {

#if defined (__linux__)

RawTraceProducer get_default_raw_producer() noexcept {
    return ::backtrace;
}

#else

// ::backtrace returns size_t on *BSD, and int on linux
static int bt_adapter(void **dst, int sz) {
    auto r = ::backtrace(dst, sz);
    return (int)r;
}

RawTraceProducer get_default_raw_producer() noexcept {
    return bt_adapter;
}
#endif


static StackframePtr as_frame (const char* symbol) {
    using guard_t = std::unique_ptr<char*, std::function<void(char**)>>;
    auto r = StackframePtr(new Stackframe());
    std::cmatch what;
#if defined (__linux__)
    static std::regex re("(.+)\\((.+)\\+(0x)?([0-9a-f]+)\\) \\[0x([0-9a-f]+)\\]");
    if (regex_match(symbol, what, re)) {
        panda::string dll           (what[1].first, what[1].length());
        panda::string mangled_name  (what[2].first, what[2].length());
        panda::string base_off      (what[3].first, what[3].length());
        panda::string symbol_offset (what[4].first, what[4].length());
        panda::string address       (what[5].first, what[5].length());
#else
#if !defined(__APPLE__)
    static std::regex re("0x([0-9a-f]+) <([^+]+)\\+(0x)?([0-9a-f]+)> at (.+)");
    if (regex_match(symbol, what, re)) {
        panda::string dll           (what[5].first, what[5].length());
        panda::string mangled_name  (what[2].first, what[2].length());
        panda::string base_off      (what[3].first, what[3].length());
        panda::string symbol_offset (what[4].first, what[4].length());
        panda::string address       (what[1].first, what[1].length());
#else
    static std::regex re("\\d+\\s+(\\S+).bundle\\s+0x([0-9a-f]+) (.+) \\+ (\\d+)");
    if (regex_match(symbol, what, re)) {
        panda::string dll           (what[1].first, what[1].length());
        panda::string mangled_name  (what[3].first, what[3].length());
        panda::string base_off;
        panda::string symbol_offset (what[4].first, what[4].length());
        panda::string address       (what[2].first, what[2].length());
#endif
#endif
        int status;
        string demangled_name = mangled_name;
        // https://en.wikipedia.org/wiki/Name_mangling, check it starts with '_Z'. The
        // abi::__cxa_demangle() on FreeBSD "demangles" even non-mangled names
        if (mangled_name.size() > 2 && (mangled_name[0] == '_') && (mangled_name[1] == 'Z')) {
            char* demangled = abi::__cxa_demangle(mangled_name.c_str(), nullptr, nullptr, &status);
            guard_t guard;
            if (demangled) {
                demangled_name = demangled;
                free(demangled);
            }
        }
        //printf("symbol = %s, d = %s, o=%s\n", symbol, demangled_name.c_str(), mangled_name.c_str());
        r->name = demangled_name;
        r->mangled_name = mangled_name;
        r->file = "n/a";
        r->library = dll;
        r->line_no = 0;

        std::uint64_t addr = 0;
        int base = base_off ? 16 : 10;
        auto addr_r = from_chars(address.data(), address.data() + address.length(), addr, base);
        if (!addr_r.ec) { r->address = addr; }
        else            { r->address = 0; }

        std::uint64_t offset = 0;
        // +2 to skip 0x prefix
        auto offset_r = from_chars(symbol_offset.data(), symbol_offset.data() + symbol_offset.size(), offset, 16);
        if (!offset_r.ec) { r->offset = offset; }
        else              { r->offset = 0; }
        //printf("symbol = %s\n", symbol);
    }
    return r;
}

struct glib_backtrace: BacktraceInfo {

    glib_backtrace(std::vector<StackframePtr>&& frames_):frames{std::move(frames_)}{}

    const std::vector<StackframePtr>& get_frames() const override { return frames; }
    virtual string to_string() const override { std::abort(); }

    std::vector<StackframePtr> frames;
};

iptr<BacktraceInfo> glibc_producer(const RawTrace& buffer) {
    using guard_t = std::unique_ptr<char**, std::function<void(char***)>>;
    char** symbols = ::backtrace_symbols(buffer.data(), buffer.size());
    if (symbols) {
        guard_t guard(&symbols, [](char*** ptr) { free(*ptr); });
        std::vector<StackframePtr> frames;
        frames.reserve(buffer.size());
        for (int i = 0; i < static_cast<int>(buffer.size()); ++i) {
            // printf("symbol = %s\n", symbols[i]);
            auto frame = as_frame(symbols[i]);
            frames.emplace_back(std::move(frame));
        }
        auto ptr = new glib_backtrace(std::move(frames));
        return iptr<BacktraceInfo>(ptr);
    }
    return iptr<BacktraceInfo>();
}


BacktraceProducer get_default_bt_producer() noexcept {
    return glibc_producer;
}

}}

#endif
