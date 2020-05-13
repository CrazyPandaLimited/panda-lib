#pragma once
#include "../pp.h"
#include "../string.h"
#include "../function.h"
//#include <iosfwd>
#include <string>
#include <memory>
#include <vector>
#include <ostream>

namespace panda { namespace log {

#define PANDA_LOG_CODE_POINT(module) panda::log::CodePoint(__FILE__, __LINE__, __func__, &(module))

#define panda_should_log(...)       PANDA_PP_VFUNC(PANDA_SHOULD_LOG, __VA_ARGS__)
#define PANDA_SHOULD_LOG1(lvl)      PANDA_SHOULD_LOG2(lvl, panda_log_module)
#define PANDA_SHOULD_LOG2(lvl, mod) ((lvl) >= (mod).level && panda::log::details::logger)
#define panda_should_rlog(lvl)      PANDA_SHOULD_LOG2(lvl, ::panda_log_module)

#define panda_log(...)            PANDA_LOG(__VA_ARGS__)                                      // proxy to expand args
#define PANDA_LOG(lvl, ...)       PANDA_PP_VFUNC(PANDA_LOG, PANDA_PP_VJOIN(lvl, __VA_ARGS__)) // separate first arg to detect empty args
#define PANDA_LOG1(lvl)           PANDA_LOG2(lvl, default_message)
#define PANDA_LOG2(lvl, msg)      PANDA_LOG3(lvl, panda_log_module, msg)
#define PANDA_LOG3(lvl, mod, msg) do {                                                                      \
    if (PANDA_SHOULD_LOG2(lvl, mod)) {                                                                      \
        std::ostream& log = panda::log::details::get_os();                                                  \
        panda::static_if<panda::log::details::IsEval<panda::log::details::getf(#msg)>::value>([&](auto) {   \
            (panda::log::details::LambdaStream&)log << msg;                                                 \
        })(panda::log::details::Unique1{});                                                                 \
        panda::static_if<!panda::log::details::IsEval<panda::log::details::getf(#msg)>::value>([&](auto) {  \
            log << msg;                                                                                     \
        })(panda::log::details::Unique2{});                                                                 \
        panda::log::details::do_log(log, PANDA_LOG_CODE_POINT(mod), lvl);                                   \
    }                                                                                                       \
} while (0)

#define panda_log_verbose_debug(...)    panda_log(panda::log::VerboseDebug, __VA_ARGS__)
#define panda_log_debug(...)            panda_log(panda::log::Debug,        __VA_ARGS__)
#define panda_log_info(...)             panda_log(panda::log::Info,         __VA_ARGS__)
#define panda_log_notice(...)           panda_log(panda::log::Notice,       __VA_ARGS__)
#define panda_log_warn(...)             panda_log(panda::log::Warning,      __VA_ARGS__)
#define panda_log_warning(...)          panda_log(panda::log::Warning,      __VA_ARGS__)
#define panda_log_error(...)            panda_log(panda::log::Error,        __VA_ARGS__)
#define panda_log_critical(...)         panda_log(panda::log::Critical,     __VA_ARGS__)
#define panda_log_alert(...)            panda_log(panda::log::Alert,        __VA_ARGS__)
#define panda_log_emergency(...)        panda_log(panda::log::Emergency,    __VA_ARGS__)

#define panda_rlog(level, msg)          panda_log(level, ::panda_log_module, msg)
#define panda_rlog_verbose_debug(msg)   panda_rlog(panda::log::VerboseDebug, msg)
#define panda_rlog_debug(msg)           panda_rlog(panda::log::Debug, msg)
#define panda_rlog_info(msg)            panda_rlog(panda::log::Info, msg)
#define panda_rlog_notice(msg)          panda_rlog(panda::log::Notice, msg)
#define panda_rlog_warn(msg)            panda_rlog(panda::log::Warning, msg)
#define panda_rlog_warning(msg)         panda_rlog(panda::log::Warning, msg)
#define panda_rlog_error(msg)           panda_rlog(panda::log::Error, msg)
#define panda_rlog_critical(msg)        panda_rlog(panda::log::Critical, msg)
#define panda_rlog_alert(msg)           panda_rlog(panda::log::Alert, msg)
#define panda_rlog_emergency(msg)       panda_rlog(panda::log::Emergency, msg)

#define panda_log_ctor(...)  PANDA_PP_VFUNC(PANDA_LOG_CTOR, __VA_ARGS__)
#define panda_log_dtor(...)  PANDA_PP_VFUNC(PANDA_LOG_DTOR, __VA_ARGS__)
#define PANDA_LOG_CTOR0()    PANDA_LOG_CTOR1(panda_log_module)
#define PANDA_LOG_CTOR1(mod) PANDA_LOG3(panda::log::VerboseDebug, mod, __func__ << " [ctor]")
#define PANDA_LOG_DTOR0()    PANDA_LOG_DTOR1(panda_log_module)
#define PANDA_LOG_DTOR1(mod) PANDA_LOG3(panda::log::VerboseDebug, mod, __func__ << " [dtor]")
#define panda_rlog_ctor()    panda_log_ctor(::panda_log_module)
#define panda_rlog_dtor()    panda_log_dtor(::panda_log_module)

#define panda_debug_v(var) panda_log(panda::log::Debug, #var << " = " << (var))

#define PANDA_ASSERT(var, msg) if(!(auto assert_value = var)) { panda_log_emergency("assert failed: " << #var << " is " << assert_value << msg) }

extern string_view default_message;

enum Level {
    VerboseDebug = 0,
    Debug,
    Info,
    Notice,
    Warning,
    Error,
    Critical,
    Alert,
    Emergency
};

struct Module {
    using Modules = std::vector<Module*>;

    Module* parent;
    Level   level;
    string  name;
    Modules children;

    Module (const string& name, Level level = Warning);
    Module (const string& name, Module& parent, Level level = Warning) : Module(name, &parent, level) {}
    Module (const string& name, Module* parent, Level level = Warning);

    Module (const Module&) = delete;
    Module (Module&&)      = delete;

    Module& operator= (const Module&) = delete;

    void set_level (Level);

    virtual ~Module ();
};

struct CodePoint {
    CodePoint () : line(), module() {}
    CodePoint (const string_view& file, uint32_t line, const string_view& func, const Module* module)
        : file(file), line(line), func(func), module(module) {}

    string_view   file;
    uint32_t      line;
    string_view   func;
    const Module* module;

    std::string to_string () const;
};

struct IFormatter : AtomicRefcnt {
    virtual string format (Level, const CodePoint&, std::string&) const = 0;
    virtual ~IFormatter () {}
};
using IFormatterSP = iptr<IFormatter>;

struct ILogger : AtomicRefcnt {
    virtual void log_format (Level, const CodePoint&, std::string&, const IFormatter&);
    virtual void log        (Level, const CodePoint&, const string&);
    virtual ~ILogger () = 0;
};
using ILoggerSP = iptr<ILogger>;

using format_fn        = function<string(Level, const CodePoint&, std::string&)>;
using logger_format_fn = function<void(Level, const CodePoint&, std::string&, const IFormatter&)>;
using logger_fn        = function<void(Level, const CodePoint&, const string&)>;

ILoggerSP    fn2logger    (const logger_format_fn&);
ILoggerSP    fn2logger    (const logger_fn&);
IFormatterSP fn2formatter (const format_fn&);

void set_level     (Level, string_view module = "");
void set_logger    (const ILoggerSP&);
void set_logger    (std::nullptr_t);
void set_formatter (const IFormatterSP&);

inline void set_logger    (const logger_format_fn& f) { set_logger(fn2logger(f)); }
inline void set_logger    (const logger_fn& f)        { set_logger(fn2logger(f)); }
inline void set_formatter (const format_fn& f)        { set_formatter(fn2formatter(f)); }

struct escaped { string_view src; };

namespace details {
    extern ILoggerSP logger;

    std::ostream& get_os ();
    bool          do_log (std::ostream&, const CodePoint&, Level);

    template <char T> struct IsEval      : std::false_type {};
    template <>       struct IsEval<'['> : std::true_type  {};

    static constexpr inline char getf (const char* s) { return *s; }


    struct LambdaStream : std::ostream {};
    struct Unique1 {};
    struct Unique2 {};

    template <class T>
    std::enable_if_t<panda::has_call_operator<T>::value, LambdaStream&> operator<< (LambdaStream& os, T&& f) {
        f();
        return os;
    }
}

std::ostream& operator<< (std::ostream&, const CodePoint&);
std::ostream& operator<< (std::ostream&, const escaped&);

}}

extern panda::log::Module panda_log_module;
