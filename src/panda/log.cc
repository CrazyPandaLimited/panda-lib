#include "log.h"
#include <mutex>
#include <math.h>
#include <time.h>
#include <memory>
#include <thread>
#include <iomanip>
#include <sstream>
#include <string.h>
#include <algorithm>
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

#ifdef _GNU_SOURCE
    extern char* program_invocation_name;
#endif

namespace panda { namespace log {

struct PatternFormatter : IFormatter {
    string_view _fmt;
    PatternFormatter (string_view fmt) : _fmt(fmt) {}
    string format (Level, const CodePoint&, std::string&) const override;
};

string_view default_format  = "%1t %c[%L/%1M]%C %f:%l,%F(): %m";
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
 * SYNTAX OF TOKEN: %X or %xX or %.yX or %x.yX
 * where x and y are optional digits (default is 0), and X is one of the following letters:
 * %L - level
 * %M - module
 *      if module has no name (root), removes x chars on the left and y chars on the right.
 * %F - function
 * %f - file
 *      x=0: only file name
 *      x=1: full path as it appeared during compilation
 * %l - line
 * %m - message
 * %t - current time
 *      x=0: YYYY/MM/DD HH:MM:SS
 *      x=1: YY/MM/DD HH:MM:SS
 *      x=2: HH:MM:SS
 *      x=3: UNIX TIMESTAMP
 *      y>0: high resolution time, adds fractional part after seconds with "y" digits precision
 * %T - current thread id
 * %p - current process id
 * %P - current process title
 * %c - start color
 * %C - end color
 */

#ifdef _WIN32
static const char* colors[] = {nullptr};
#else
static const char* colors[] = {nullptr, nullptr, nullptr, nullptr, "\e[33m", "\e[31m", "\e[41m", "\e[41m", "\e[41m"};
#endif
static const char  clear_color[] = "\e[0m";
static const char* dates[] = {"%Y-%m-%d %H:%M:%S", "%y-%m-%d %H:%M:%S", "%H:%M:%S"};

static void add_mks (string& dest, long nsec, unsigned prec) {
    dest += '.';
    if (nsec < 100000000) dest += '0';
    if (nsec < 10000000)  dest += '0';
    if (nsec < 1000000)   dest += '0';
    if (nsec < 100000)    dest += '0';
    if (nsec < 10000)     dest += '0';
    if (nsec < 1000)      dest += '0';
    if (nsec < 100)       dest += '0';
    if (nsec < 10)        dest += '0';
    dest += panda::to_string(nsec);
    dest.length(dest.length() - 9 + prec);
}

static inline struct timespec now (unsigned hires) {
    struct timespec ts;
    if (hires) {
        int status = clock_gettime(CLOCK_REALTIME, &ts);
        if (status != 0) {
            ts.tv_sec = 0;
            ts.tv_nsec = 0;
        }
    } else {
        ts.tv_sec = std::time(nullptr);
        ts.tv_nsec = 0;
    }
    return ts;
}

static inline void ymdhms (string& dest, time_t epoch, unsigned type) {
    struct tm dt;
    if (!_PANDA_LOCALTIME(&epoch, &dt)) return;

    auto cap = 23;
    string tmp(cap);

    assert(type < 3);
    auto len = strftime(tmp.buf(), cap, dates[type], &dt);
    tmp.length(len);

    dest += tmp;
}

#ifdef _GNU_SOURCE
    static inline const char* get$0 () { return program_invocation_name; }
#else
    static inline const char* get$0 () { return "<unknown>"; }
#endif

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

        unsigned x = 0, y = 0;
        bool dot = false;

        SWITCH:
        switch (*s++) {
            case 'F': {
                if (cp.func.length()) ret += cp.func;
                else                  ret += "<top>";
                break;
            }
            case 'f': {
                if (x) ret += cp.file;
                else {
                    auto file = cp.file;
                    auto pos = file.rfind('/');
                    if (pos < file.length()) file = file.substr(pos+1);
                  #ifdef _WIN32
                    pos = file.rfind('\\');
                    if (pos < file.length()) file = file.substr(pos+1);
                  #endif
                    ret += file;
                }
                break;
            }
            case 'l': ret += panda::to_string(cp.line);             break;
            case 'm': ret += string_view(msg.data(), msg.length()); break;
            case 'M': {
                if (cp.module->name.length()) ret += cp.module->name;
                else {
                    if (x && ret.length() >= x) ret.length(ret.length() - x);
                    s += y;
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
            case 't': {
                auto t = now(y);
                if (x < 3) ymdhms(ret, t.tv_sec, x);
                else       ret += panda::to_string(t.tv_sec);
                if (y) add_mks(ret, t.tv_nsec, y);
                break;
            }
            case 'T': {
                std::stringstream ss;
                ss << std::this_thread::get_id();
                auto str = ss.str();
                ret.append(str.data(), str.length());
                break;
            }
            case 'p': {
                ret += panda::to_string(_PANDA_GETPID);
                break;
            }
            case 'P': {
                ret += get$0();
                break;
            }
            case 'c': if (colors[level]) ret += colors[level]; break;
            case 'C': if (colors[level]) ret += clear_color; break;
            case '.': {
                if (s == se) throw exception("bad formatter pattern");
                dot = true;
                goto SWITCH;
            }
            default: {
                char c = *(s-1);
                if (s == se || c < '0' || c > '9') throw exception("bad formatter pattern");
                (dot ? y : x) = c - '0';
                goto SWITCH;
            }
        }
    }

    return ret;
}

}}

panda::log::Module panda_log_module("", nullptr);

