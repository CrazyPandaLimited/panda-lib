#pragma once
#include "pp.h"
#include "string.h"
#include "function.h"
//#include <iosfwd>
#include <string>
#include <memory>
#include <vector>
#include <ostream>
#include <string.h>

namespace panda { namespace log {

#ifdef WIN32
#  define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#  define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define PANDA_LOG_CODE_POINT(module) panda::log::CodePoint{__FILENAME__, __LINE__, __func__, &module}

#define panda_should_log(...)       PANDA_PP_VFUNC(panda_should_log, __VA_ARGS__)
#define panda_should_log1(lvl)      panda_should_log2(lvl, panda_log_module)
#define panda_should_log2(lvl, mod) (lvl >= mod.level && panda::log::details::logger)
#define panda_should_rlog(lvl)      panda_should_log(lvl, ::panda_log_module)

#define panda_elog(...)             PANDA_PP_VFUNC(panda_elog, __VA_ARGS__)
#define panda_elog2(lvl, code)      panda_elog3(lvl, panda_log_module, code)
#define panda_elog3(lvl, mod, code) do {                                    \
    if (panda_should_log2(lvl, mod)) {                                      \
        std::ostream& log = panda::log::details::get_os();                  \
        code;                                                               \
        panda::log::details::do_log(log, PANDA_LOG_CODE_POINT(mod), lvl);   \
    }                                                                       \
} while (0)

#define panda_log(...)                 PANDA_PP_VFUNC(panda_log, __VA_ARGS__)
#define panda_log1(level)              panda_log2(level, "")
#define panda_log2(level, msg)         panda_log3(level, panda_log_module, msg)
#define panda_log3(level, module, msg) panda_elog3(level, module, { log << msg; })

#define panda_rlog(level, msg)          panda_log(level, ::panda_log_module, msg)

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

#define panda_elog_verbose_debug(...)  panda_elog(panda::log::VerboseDebug, __VA_ARGS__)
#define panda_elog_debug(...)          panda_elog(panda::log::Debug, __VA_ARGS__)
#define panda_elog_info(...)           panda_elog(panda::log::Info, __VA_ARGS__)
#define panda_elog_notice(...)         panda_elog(panda::log::Notice, __VA_ARGS__)
#define panda_elog_warn(...)           panda_elog(panda::log::Warning, __VA_ARGS__)
#define panda_elog_warning(...)        panda_elog(panda::log::Warning, __VA_ARGS__)
#define panda_elog_error(...)          panda_elog(panda::log::Error, __VA_ARGS__)
#define panda_elog_critical(...)       panda_elog(panda::log::Critical, __VA_ARGS__)
#define panda_elog_alert(...)          panda_elog(panda::log::Alert, __VA_ARGS__)
#define panda_elog_emergency(...)      panda_elog(panda::log::Emergency, __VA_ARGS__)

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

#define panda_mlog_ctor(mod)    panda_log_verbose_debug(mod, __func__ << " [ctor]")
#define panda_mlog_dtor(mod)    panda_log_verbose_debug(mod, __func__ << " [dtor]")
#define panda_log_ctor()        panda_mlog_ctor(panda_log_module)
#define panda_log_dtor()        panda_mlog_dtor(panda_log_module)
#define panda_rlog_ctor()       panda_mlog_ctor(::panda_log_module)
#define panda_rlog_dtor()       panda_mlog_dtor(::panda_log_module)

#define panda_debug_v(var) panda_log(panda::log::Debug, #var << " = " << (var))

#define PANDA_ASSERT(var, msg) if(!(auto assert_value = var)) { panda_log_emergency("assert failed: " << #var << " is " << assert_value << msg) }

extern string_view default_format;

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

void set_level     (Level, string_view module = "");
void set_logger    (const ILoggerSP&);
void set_logger    (const logger_format_fn&);
void set_logger    (const logger_fn&);
void set_logger    (std::nullptr_t);
void set_formatter (const IFormatterSP&);
void set_formatter (const format_fn&);
void set_format    (string_view pattern);

struct escaped { string_view src; };

namespace details {
    extern ILoggerSP logger;

    std::ostream& get_os ();
    bool          do_log (std::ostream&, const CodePoint&, Level);
}

std::ostream& operator<< (std::ostream&, const CodePoint&);
std::ostream& operator<< (std::ostream&, const escaped&);

}}

extern panda::log::Module panda_log_module;
