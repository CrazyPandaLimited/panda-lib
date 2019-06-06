#pragma once
#include <string>
#include <memory>
#include <string.h>
#include <type_traits>
#include <panda/string_view.h>

namespace panda { namespace log {

#ifdef WIN323
#  define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#  define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define _panda_log_code_point_ panda::log::CodePoint{__FILENAME__, __LINE__}

#define panda_should_log(LEVEL) panda::log::should_log(LEVEL, _panda_log_code_point_)

#define panda_elog(LEVEL, CODE) do {                                        \
    if (panda::log::should_log(LEVEL, _panda_log_code_point_)) {            \
        std::ostream& log = panda::log::details::_get_os();                 \
        CODE;                                                               \
        panda::log::details::_do_log(log, _panda_log_code_point_, LEVEL);   \
    }                                                                       \
} while (0);

#define panda_log(LEVEL, MSG) panda_elog(LEVEL, { log << MSG; })

#define panda_log_verbose_debug(MSG)   panda_log(panda::log::VERBOSE_DEBUG, MSG)
#define panda_log_debug(MSG)           panda_log(panda::log::DEBUG, MSG)
#define panda_log_info(MSG)            panda_log(panda::log::INFO, MSG)
#define panda_log_notice(MSG)          panda_log(panda::log::NOTICE, MSG)
#define panda_log_warn(MSG)            panda_log(panda::log::WARNING, MSG)
#define panda_log_error(MSG)           panda_log(panda::log::ERROR, MSG)
#define panda_log_critical(MSG)        panda_log(panda::log::CRITICAL, MSG)
#define panda_log_alert(MSG)           panda_log(panda::log::ALERT, MSG)
#define panda_log_emergency(MSG)       panda_log(panda::log::EMERGENCY, MSG)
#define panda_elog_verbose_debug(CODE) panda_elog(panda::log::VERBOSE_DEBUG, CODE)
#define panda_elog_debug(CODE)         panda_elog(panda::log::DEBUG, CODE)
#define panda_elog_info(CODE)          panda_elog(panda::log::INFO, CODE)
#define panda_elog_notice(CODE)        panda_elog(panda::log::NOTICE, CODE)
#define panda_elog_warn(CODE)          panda_elog(panda::log::WARNING, CODE)
#define panda_elog_error(CODE)         panda_elog(panda::log::ERROR, CODE)
#define panda_elog_critical(CODE)      panda_elog(panda::log::CRITICAL, CODE)
#define panda_elog_alert(CODE)         panda_elog(panda::log::ALERT, CODE)
#define panda_elog_emergency(CODE)     panda_elog(panda::log::EMERGENCY, CODE)

#define panda_debug_v(VAR) panda_log(panda::log::DEBUG, #VAR << " = " << (VAR))

#define PANDA_ASSERT(VAR, MSG) if(!(auto assert_value = VAR)) { panda_log_emergency("assert failed: " << #VAR << " is " << assert_value << MSG) }

enum Level {
    VERBOSE_DEBUG = 0,
    DEBUG,
    INFO,
    NOTICE,
    WARNING,
    ERROR,
    CRITICAL,
    ALERT,
    EMERGENCY
};

struct CodePoint {
    std::string_view file;
    uint32_t         line;

    std::string to_string () const;
};
std::ostream& operator<< (std::ostream&, const CodePoint&);

struct ILogger {
    virtual bool should_log (Level, const CodePoint&) { return true; }
    virtual void log        (Level, const CodePoint&, const std::string&) = 0;
};

namespace details {
    extern Level                    min_level;
    extern std::unique_ptr<ILogger> ilogger;

    std::ostream& _get_os ();
    bool          _do_log (std::ostream&, const CodePoint&, Level);

    template <class Func>
    struct CallbackLogger : ILogger {
        Func f;
        CallbackLogger (const Func& f) : f(f) {}
        void log (Level level, const CodePoint& cp, const std::string& s) override { f(level, cp, s); }
    };
}

void set_level  (Level);
void set_logger (ILogger*);

template <class Func, typename = std::enable_if_t<!std::is_base_of<ILogger, std::remove_cv_t<std::remove_pointer_t<Func>>>::value>>
void set_logger (const Func& f) { set_logger(new details::CallbackLogger<Func>(f)); }

inline bool should_log (Level level, const CodePoint& cp) { return level >= details::min_level && details::ilogger && details::ilogger->should_log(level, cp); }

struct escaped {
    std::string_view src;
};
std::ostream& operator<< (std::ostream&, const escaped&);

}}

