#include "log.h"
#include <math.h>
#include <memory>
#include <thread>
#include <iomanip>
#include <sstream>

panda::log::Module panda_log_module("", nullptr);

namespace panda { namespace log {

namespace details {
    std::shared_ptr<ILogger> ilogger;

    static thread_local struct { std::ostringstream os; } tls; // struct folding workarounds a bug in FreeBSD with TLS
    static thread_local std::ostringstream* os = &tls.os;      // stream for child threads, TLS via pointers works 3x faster in GCC
    static std::ostringstream mt_os;                           // stream for main thread, can't use TLS because it's destroyed much earlier
    static auto mt_id = std::this_thread::get_id();

    std::ostream& get_os () { return std::this_thread::get_id() == mt_id ? mt_os : *os; }

    bool do_log (std::ostream& _stream, const CodePoint& cp, Level level) {
        std::ostringstream& stream = static_cast<std::ostringstream&>(_stream);
        stream.flush();
        std::string s(stream.str());
        stream.str({});
        auto hold = ilogger;
        if (hold) hold->log(level, cp, s);
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

void set_logger (const std::shared_ptr<ILogger>& l) {
    details::ilogger = l;
}

void set_logger (std::nullptr_t) {
    details::ilogger.reset();
}

void set_logger (const logger_fn& f) {
    struct Logger : ILogger {
        logger_fn f;
        Logger (const logger_fn& f) : f(f) {}
        void log (Level level, const CodePoint& cp, const std::string& s) override { f(level, cp, s); }
    };

    set_logger(std::make_shared<Logger>(f));
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

}}
