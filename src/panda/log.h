#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <panda/function.h>
#include <panda/CallbackDispatcher.h>
#include <string.h>
#include <chrono>

namespace panda {
class Log
{
public:
    enum Level {
        EMERGENCY,
        CRITICAL,
        WARNING,
        INFO,
        VERBOSE,
        DEBUG
    };

    Log(Level log_level = DEBUG)
        : level(log_level)
    {}

    template <typename T>
    std::ostringstream& operator <<(T&& t)
    {
        os << t;
        return os;
    }

    ~Log()
    {
        os.flush();
        s = os.str();

        loggers()(level, s);
    }

    using LogFunction = void (Level, const std::string&);
    using Dispatcher = CallbackDispatcher<LogFunction>;

    static Dispatcher& loggers();

private:
    std::string s;
    std::ostringstream os;
    Level level;
};
#ifdef WIN323
#  define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#  define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif // WIN32


#define _panda_log_impl_(LEVEL, MSG) panda::Log(panda::Log::LEVEL) << __FILENAME__ << " " << __LINE__ << " " << MSG

//#define panda_debug_v(VAR) Log(Log::DEBUG) << __FILENAME__ << " " << __LINE__ << " " << #VAR << " = " << (VAR)
#define panda_debug_v(VAR) _panda_log_impl_(DEBUG, #VAR << " = " << (VAR))

#define panda_log_debug(MSG) _panda_log_impl_(DEBUG, MSG)
#define panda_log_verbose(MSG) _panda_log_impl_(VERBOSE, MSG)
#define panda_log_info(MSG) _panda_log_impl_(INFO, MSG)
#define panda_log_warn(MSG) _panda_log_impl_(WARNING, MSG)

#define panda_log_critical(MSG) _panda_log_impl_(CRITICAL, MSG)
#define panda_log_emergency(MSG) _panda_log_impl_(EMERGENCY, MSG)


#define PANDA_ASSERT(VAR, MSG) if(!(auto assert_value = VAR)) {panda_log_emergency("assert failed: " << #VAR << " is " << assert_value << MSG)}

}
