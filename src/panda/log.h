#pragma once
#include <string>
#include <memory>
#include <ostream>
#include <string.h>
#include "function.h"
#include "string_map.h"

namespace panda { namespace log {

#ifdef WIN32
#  define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#  define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define _panda_log_code_point_(module) panda::log::CodePoint{__FILENAME__, __LINE__, __func__, &module}

#define panda_should_mlog(mod, lvl) (lvl >= mod.level && panda::log::details::ilogger && panda::log::details::ilogger->should_log(lvl, _panda_log_code_point_(mod)))
#define panda_should_log(lvl)       panda_should_mlog(panda_log_module, lvl)
#define panda_should_rlog(lvl)      panda_should_mlog(::panda_log_module, lvl)

#define panda_emlog(mod, lvl, code) do {                                    \
    if (panda_should_mlog(mod, lvl)) {                                      \
        std::ostream& log = panda::log::details::get_os();                  \
        code;                                                               \
        panda::log::details::do_log(log, _panda_log_code_point_(mod), lvl); \
    }                                                                       \
} while (0)

#define panda_mlog(module, level, msg)  panda_emlog(module, level, { log << msg; })
#define panda_elog(level, code)         panda_emlog(panda_log_module, level, code)
#define panda_log(level, msg)           panda_mlog(panda_log_module, level, msg)
#define panda_rlog(level, msg)          panda_mlog(::panda_log_module, level, msg)

#define panda_log_verbose_debug(msg)    panda_log(panda::log::VerboseDebug, msg)
#define panda_log_debug(msg)            panda_log(panda::log::Debug, msg)
#define panda_log_info(msg)             panda_log(panda::log::Info, msg)
#define panda_log_notice(msg)           panda_log(panda::log::Notice, msg)
#define panda_log_warn(msg)             panda_log(panda::log::Warning, msg)
#define panda_log_warning(msg)          panda_log(panda::log::Warning, msg)
#define panda_log_error(msg)            panda_log(panda::log::Error, msg)
#define panda_log_critical(msg)         panda_log(panda::log::Critical, msg)
#define panda_log_alert(msg)            panda_log(panda::log::Alert, msg)
#define panda_log_emergency(msg)        panda_log(panda::log::Emergency, msg)

#define panda_elog_verbose_debug(code)  panda_elog(panda::log::VerboseDebug, code)
#define panda_elog_debug(code)          panda_elog(panda::log::Debug, code)
#define panda_elog_info(code)           panda_elog(panda::log::Info, code)
#define panda_elog_notice(code)         panda_elog(panda::log::Notice, code)
#define panda_elog_warn(code)           panda_elog(panda::log::Warning, code)
#define panda_elog_warning(code)        panda_elog(panda::log::Warning, code)
#define panda_elog_error(code)          panda_elog(panda::log::Error, code)
#define panda_elog_critical(code)       panda_elog(panda::log::Critical, code)
#define panda_elog_alert(code)          panda_elog(panda::log::Alert, code)
#define panda_elog_emergency(code)      panda_elog(panda::log::Emergency, code)

#define panda_mlog_verbose_debug(module, msg)   panda_mlog(module, panda::log::VerboseDebug, msg)
#define panda_mlog_debug(module, msg)           panda_mlog(module, panda::log::Debug, msg)
#define panda_mlog_info(module, msg)            panda_mlog(module, panda::log::Info, msg)
#define panda_mlog_notice(module, msg)          panda_mlog(module, panda::log::Notice, msg)
#define panda_mlog_warn(module, msg)            panda_mlog(module, panda::log::Warning, msg)
#define panda_mlog_warning(module, msg)         panda_mlog(module, panda::log::Warning, msg)
#define panda_mlog_error(module, msg)           panda_mlog(module, panda::log::Error, msg)
#define panda_mlog_critical(module, msg)        panda_mlog(module, panda::log::Critical, msg)
#define panda_mlog_alert(module, msg)           panda_mlog(module, panda::log::Alert, msg)
#define panda_mlog_emergency(module, msg)       panda_mlog(module, panda::log::Emergency, msg)

#define panda_rlog_verbose_debug(msg)   panda_mlog(::panda_log_module, panda::log::VerboseDebug, msg)
#define panda_rlog_debug(msg)           panda_mlog(::panda_log_module, panda::log::Debug, msg)
#define panda_rlog_info(msg)            panda_mlog(::panda_log_module, panda::log::Info, msg)
#define panda_rlog_notice(msg)          panda_mlog(::panda_log_module, panda::log::Notice, msg)
#define panda_rlog_warn(msg)            panda_mlog(::panda_log_module, panda::log::Warning, msg)
#define panda_rlog_warning(msg)         panda_mlog(::panda_log_module, panda::log::Warning, msg)
#define panda_rlog_error(msg)           panda_mlog(::panda_log_module, panda::log::Error, msg)
#define panda_rlog_critical(msg)        panda_mlog(::panda_log_module, panda::log::Critical, msg)
#define panda_rlog_alert(msg)           panda_mlog(::panda_log_module, panda::log::Alert, msg)
#define panda_rlog_emergency(msg)       panda_mlog(::panda_log_module, panda::log::Emergency, msg)

#define panda_mlog_ctor(mod)    panda_mlog_verbose_debug(mod, __func__ << " [ctor]")
#define panda_mlog_dtor(mod)    panda_mlog_verbose_debug(mod, __func__ << " [dtor]")
#define panda_log_ctor()        panda_mlog_ctor(panda_log_module)
#define panda_log_dtor()        panda_mlog_dtor(panda_log_module)
#define panda_rlog_ctor()       panda_mlog_ctor(::panda_log_module)
#define panda_rlog_dtor()       panda_mlog_dtor(::panda_log_module)

#define panda_debug_v(var) panda_log(panda::log::Debug, #var << " = " << (var))

#define PANDA_ASSERT(var, msg) if(!(auto assert_value = var)) { panda_log_emergency("assert failed: " << #var << " is " << assert_value << msg) }

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
    using Modules = string_map<string, Module*>;

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
};

struct CodePoint {
    string_view   file;
    uint32_t      line;
    string_view   func;
    const Module* module;

    std::string to_string () const;
};

struct ILogger {
    virtual bool should_log (Level, const CodePoint&) { return true; }
    virtual void log        (Level, const CodePoint&, const std::string&) = 0;

    virtual ~ILogger () {}
};

using logger_fn = function<void(Level, const CodePoint&, const std::string&)>;

void set_level  (Level, string_view module = "");
void set_logger (const std::shared_ptr<ILogger>&);
void set_logger (const logger_fn&);
void set_logger (std::nullptr_t);

struct escaped { string_view src; };

namespace details {
    extern Level                    min_level;
    extern std::shared_ptr<ILogger> ilogger;

    std::ostream& get_os ();
    bool          do_log (std::ostream&, const CodePoint&, Level);
}

std::ostream& operator<< (std::ostream&, const CodePoint&);
std::ostream& operator<< (std::ostream&, const escaped&);

}}

extern panda::log::Module panda_log_module;
