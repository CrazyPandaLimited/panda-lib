#include "log.h"
#include <math.h>
#include <time.h>
#include <memory>
#include <thread>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <mutex>
#include "exception.h"
#include "unordered_string_map.h"

#ifdef _WIN32
    #include <process.h>
    #define _PANDA_GETPID _getpid()
    #define _PANDA_LOCALTIME(epoch_ptr, tm_ptr) (localtime_s(tm_ptr, epoch_ptr) == 0)
#else
    #include <unistd.h>
    #define _PANDA_GETPID getpid()
    #define _PANDA_LOCALTIME(epoch_ptr, tm_ptr) (localtime_r(epoch_ptr, tm_ptr) != nullptr)
#endif

panda::log::Module panda_log_module("", nullptr);

namespace panda { namespace log {

struct PatternFormatter : IFormatter {
    string_view _fmt;
    PatternFormatter (string_view fmt) : _fmt(fmt) {}
    string format (Level, const CodePoint&, std::string&) const override;
};

string_view default_format  = "[%L/%M] %f:%l:%F(): %m";
string_view default_message = "==> MARK <==";

ILogger::~ILogger () {}

namespace details {
    using Modules = unordered_string_multimap<string, Module*>;

    struct Data {
        ILoggerSP          logger;
        IFormatterSP       formatter;
        std::ostringstream os;
    };

    ILoggerSP logger;

    static std::recursive_mutex mtx;
    #define LOG_LOCK std::lock_guard<decltype(mtx)> guard(mtx);
    #define MOD_LOCK std::lock_guard<decltype(mtx)> guard(mtx);

    static IFormatterSP formatter = IFormatterSP(new PatternFormatter(default_format));
    static Modules      modules;
    static auto         mt_id = std::this_thread::get_id();
    static Data         mt_data; // data for main thread, can't use TLS because it's destroyed much earlier

    static thread_local Data  _ct_data;            // data for child threads
    static thread_local auto* ct_data = &_ct_data; // TLS via pointers works 3x faster in GCC

    static Data& get_data () { return std::this_thread::get_id() == mt_id ? mt_data : *ct_data; }

    std::ostream& get_os () { return get_data().os; }

    bool do_log (std::ostream& _stream, const CodePoint& cp, Level level) {
        std::ostringstream& stream = static_cast<std::ostringstream&>(_stream);
        stream.flush();
        std::string s(stream.str());
        stream.str({});

        auto& data = get_data();

        if (data.logger != logger) {
            LOG_LOCK;
            data.logger = logger;
        }
        if (data.formatter != formatter) {
            LOG_LOCK;
            data.formatter = formatter;
        }

        if (data.logger) {
            data.logger->log_format(level, cp, s, *(data.formatter));
        }
        return true;
    }
}
using namespace details;

void ILogger::log_format (Level level, const CodePoint& cp, std::string& s, const IFormatter& fmt) {
    log(level, cp, fmt.format(level, cp, s));
}

void ILogger::log (Level, const CodePoint&, const string&) {
    assert(0 && "either ILogger::log or ILogger::log_format must be implemented");
}

void set_logger (const ILoggerSP& l) {
    LOG_LOCK;
    logger = get_data().logger = l;
}

void set_logger (std::nullptr_t) {
    LOG_LOCK;
    logger.reset();
    get_data().logger.reset();
}

void set_logger (const logger_format_fn& f) {
    struct Logger : ILogger {
        logger_format_fn f;
        Logger (const logger_format_fn& f) : f(f) {}
        void log_format (Level level, const CodePoint& cp, std::string& s, const IFormatter& fmt) override { f(level, cp, s, fmt); }
    };
    set_logger(new Logger(f));
}

void set_logger (const logger_fn& f) {
    struct Logger : ILogger {
        logger_fn f;
        Logger (const logger_fn& f) : f(f) {}
        void log (Level level, const CodePoint& cp, const string& s) override { f(level, cp, s); }
    };
    set_logger(new Logger(f));
}

void set_formatter (const IFormatterSP& f) {
    if (!f) return set_format(default_format);
    LOG_LOCK;
    formatter = get_data().formatter = f;
}

void set_formatter (const format_fn& f) {
    struct Formatter : IFormatter {
        format_fn f;
        Formatter (const format_fn& f) : f(f) {}
        string format (Level level, const CodePoint& cp, std::string& s) const override { return f(level, cp, s); }
    };
    set_formatter(new Formatter(f));
}

void set_format (string_view pattern) {
    set_formatter(new PatternFormatter(pattern));
}

Module::Module (const string& name, Level level) : Module(name, panda_log_module, level) {}

Module::Module (const string& name, Module* parent, Level level) : parent(parent), level(level), name(name) {
    MOD_LOCK;

    if (parent) {
        parent->children.push_back(this);
        if (parent->name) this->name = parent->name + "::" + name;
    }

    if (this->name) modules.emplace(this->name, this);
}

void Module::set_level (Level level) {
    MOD_LOCK;
    this->level = level;
    for (auto& m : children) m->set_level(level);
}

Module::~Module () {
    MOD_LOCK;
    for (auto& m : children) m->parent = nullptr;
    if (parent) {
        auto it = std::find(parent->children.begin(), parent->children.end(), this);
        assert(it != parent->children.end());
        parent->children.erase(it);
    }

    if (name) {
        auto range = modules.equal_range(name);
        while (range.first != range.second) {
            if (range.first->second != this) {
                ++range.first;
                continue;
            }
            modules.erase(range.first);
            break;
        }
    }
}

void set_level (Level val, string_view modname) {
    MOD_LOCK;
    if (!modname.length()) return ::panda_log_module.set_level(val);

    auto range = modules.equal_range(modname);
    if (range.first == range.second) throw exception(string("unknown module: ") + modname);

    while (range.first != range.second) {
        range.first->second->set_level(val);
        ++range.first;
    }
}

std::string CodePoint::to_string () const {
    std::ostringstream os;
    os << *this;
    os.flush();
    return os.str();
}

std::ostream& operator<< (std::ostream& stream, const CodePoint& cp) {
    size_t total = cp.file.size() + log10(cp.line) + 2;
    const char* whitespaces = "                        "; // 24 spaces
    if (total < 24) {
        whitespaces += total;
    } else {
        whitespaces = "";
    }
    stream << cp.file << ":" << cp.line << whitespaces;
    return stream;
}

std::ostream& operator<< (std::ostream& stream, const escaped& str) {
   for (auto c : str.src) {
       if (c > 31) {
           stream << c;
       } else {
           stream << "\\" << std::setfill('0') << std::setw(2) << uint32_t(uint8_t(c));
       }
   }
   return stream;
}

/*
 * %L - level
 * %M - module
 * %F - function
 * %f - file
 * %l - line
 * %m - message
 * %d - current datetime (YYYY/MM/DD HH:MM:SS)
 * %D - current datetime hires (YYYY/MM/DD HH:MM:SS.SSS)
 * %t - current time (HH:MM:SS)
 * %T - current time hires (HH:MM:SS.SSS)
 * %p - current process id
 * %P - current thread id
 */

static void add_mks (string& dest, long nsec) {
    dest += '.';
    auto mksec = nsec / 1000;
    if (mksec < 100000) dest += '0';
    if (mksec < 10000)  dest += '0';
    if (mksec < 1000)   dest += '0';
    if (mksec < 100)    dest += '0';
    if (mksec < 10)     dest += '0';
    dest += panda::to_string(mksec);
}

static inline struct timespec now_hires () {
    struct timespec ts;
    int status = clock_gettime(CLOCK_REALTIME, &ts);
    if (status != 0) {
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
    }
    return ts;
}

static inline time_t now () { return std::time(nullptr); }

static inline void ymdhms (string& dest, time_t epoch) {
    struct tm dt;
    if (!_PANDA_LOCALTIME(&epoch, &dt)) return;

    auto cap = 23;
    string tmp(cap);

    auto len = strftime(tmp.buf(), cap, "%Y-%m-%d %H:%M:%S", &dt);
    tmp.length(len);

    dest += tmp;
}

static inline void hms (string& dest, time_t epoch) {
    struct tm dt;
    if (!_PANDA_LOCALTIME(&epoch, &dt)) return;

    auto cap = 23;
    string tmp(cap);

    auto len = strftime(tmp.buf(), cap, "%H:%M:%S", &dt);
    tmp.length(len);

    dest += tmp;
}

string PatternFormatter::format (Level level, const CodePoint& cp, std::string& msg) const {
    auto len = _fmt.length();
    auto s   = _fmt.data();
    auto se  = s + len;
    string ret((len > 25 ? len*2 : 50) + msg.length());

    while (s < se) {
        char c = *s++;
        if (c != '%' || s == se) {
            ret += c;
            continue;
        }
        switch (*s++) {
            case 'F': ret += cp.func;                               break;
            case 'f': ret += cp.file;                               break;
            case 'l': ret += panda::to_string(cp.line);             break;
            case 'm': ret += string_view(msg.data(), msg.length()); break;
            case 'M': {
                if (cp.module->name.length()) {
                    ret += cp.module->name;
                } else {
                    if (s-3 >= _fmt.data() && *(s-3) == '/') ret.pop_back();
                }
                break;
            }
            case 'L': {
                switch (level) {
                    case VerboseDebug : ret += "DEBUG";     break;
                    case Debug        : ret += "debug";     break;
                    case Info         : ret += "info";      break;
                    case Notice       : ret += "notice";    break;
                    case Warning      : ret += "warning";   break;
                    case Error        : ret += "error";     break;
                    case Critical     : ret += "critical";  break;
                    case Alert        : ret += "alert";     break;
                    case Emergency    : ret += "emergency"; break;
                    default: break;
                }
                break;
            }
            case 'd': {
                ymdhms(ret, now());
                break;
            }
            case 'D': {
                auto now = now_hires();
                ymdhms(ret, now.tv_sec);
                add_mks(ret, now.tv_nsec);
                //auto now = date::Date::now_hires();
                //ret += now.to_string();
                break;
            }
            case 't': {
                hms(ret, now());
                break;
            }
            case 'T': {
                auto now = now_hires();
                hms(ret, now.tv_sec);
                add_mks(ret, now.tv_nsec);
                break;
            }
            case 'p': {
                ret += panda::to_string(_PANDA_GETPID);
                break;
            }
            case 'P': {
                std::stringstream ss;
                ss << std::this_thread::get_id();
                auto str = ss.str();
                ret.append(str.data(), str.length());
                break;
            }
            case 'u': {
                ret += panda::to_string(now());
                break;
            }
            case 'U': {
                auto now = now_hires();
                ret += panda::to_string(now.tv_sec);
                add_mks(ret, now.tv_nsec);
                break;
            }
            default: ret += *(s-1); // keep symbol after percent
        }
    }

    return ret;
}

}}
