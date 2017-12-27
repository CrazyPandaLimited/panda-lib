#include "log.h"

namespace panda {

Log::Dispatcher& Log::loggers() {
    static Dispatcher inst;
    return inst;
}

Log::FilterFunction _filter_function;

void Log::set_filter(const Log::FilterFunction& filter) {
    _filter_function = filter;
}

bool Log::should_log(Log::Level level, logger::CodePoint cp) {
    if (_filter_function) {
        return _filter_function(level, cp);
    } else {
        return true;
    }
}

}
