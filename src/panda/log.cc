#include "log.h"
#include <math.h>
#include <memory>
#include <thread>
#include <iomanip>
#include <sstream>
#include <mutex>

panda::log::Module panda_log_module("", nullptr);

namespace panda { namespace log {

struct PatternFormatter : IFormatter {
    string_view _fmt;
    PatternFormatter (string_view fmt) : _fmt(fmt) {}
    string format (Level, const CodePoint&, std::string&) const override;
};

string_view default_format = "[%L/%M] %f:%l:%F(): %m";

ILogger::~ILogger () {}

namespace details {
    ILoggerSP    logger;
    IFormatterSP formatter = IFormatterSP(new PatternFormatter(default_format));

    struct Data {
        ILoggerSP          logger;
        IFormatterSP       formatter;
        std::ostringstream os;
    };

    static Data mt_data; // data for main thread, can't use TLS because it's destroyed much earlier
    static auto mt_id = std::this_thread::get_id();

    static thread_local Data  _ct_data;            // data for child threads
    static thread_local auto* ct_data = &_ct_data; // TLS via pointers works 3x faster in GCC

    static std::mutex mtx;

    std::ostream& get_os () { return std::this_thread::get_id() == mt_id ? mt_data.os : ct_data->os; }

    bool do_log (std::ostream& _stream, const CodePoint& cp, Level level) {
        std::ostringstream& stream = static_cast<std::ostringstream&>(_stream);
        stream.flush();
        std::string s(stream.str());
        stream.str({});

        auto& data = std::this_thread::get_id() == mt_id ? mt_data : *ct_data;

        if (data.logger != logger) {
            std::lock_guard<std::mutex> guard(mtx);
            data.logger = logger;
        }
        if (data.formatter != formatter) {
            std::lock_guard<std::mutex> guard(mtx);
            data.formatter = formatter;
        }

        if (data.logger) {
            data.logger->log_format(level, cp, s, *(data.formatter));
        }
        return true;
    }
}

void set_level (Level val, string_view module) {
    if (module.length()) {
        auto& modules = ::panda_log_module.children;
        auto iter = modules.find(module);
        if (iter == modules.end()) {
            throw std::invalid_argument("unknown module");
        }
        iter->second->set_level(val);
    } else {
        panda_log_module.set_level(val);
    }
}

void ILogger::log_format (Level level, const CodePoint& cp, std::string& s, const IFormatter& fmt) {
    log(level, cp, fmt.format(level, cp, s));
}

void ILogger::log (Level, const CodePoint&, const string&) {
    assert(0 && "either ILogger::log or ILogger::log_format must be implemented");
}

void set_logger (const ILoggerSP& l) {
    std::lock_guard<std::mutex> guard(details::mtx);
    details::logger = l;
}

void set_logger (std::nullptr_t) {
    std::lock_guard<std::mutex> guard(details::mtx);
    details::logger.reset();
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
    assert(f);
    std::lock_guard<std::mutex> guard(details::mtx);
    details::formatter = f;
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

Module::Module (const string& name, Level level) : Module(name, panda_log_module, level) {}

Module::Module (const string& name, Module* parent, Level level) : level(level), name(name) {
    if (!parent) return;

    this->parent = parent;

    if (parent->children.find(name) != parent->children.end()) {
        string msg = "panda::log::Module " + name + "is already registered";
        throw std::logic_error(msg.c_str());
    }
    parent->children[name] = this;
}

void Module::set_level (Level level) {
    this->level = level;
    for (auto& p : children) {
        p.second->set_level(level);
    }
}

/*
 * %L - level
 * %M - module
 * %F - function
 * %f - file
 * %l - line
 * %m - message
 */
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
            case 'M':
                if (cp.module->name.length()) {
                    ret += cp.module->name;
                } else {
                    if (s-3 >= _fmt.data() && *(s-3) == '/') ret.pop_back();
                }
                break;
            case 'L':
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
            default: ret += *(s-1); // keep symbol after percent
        }
    }

    return ret;
}

}}
