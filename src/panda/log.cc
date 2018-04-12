#include "log.h"

namespace panda {

static logger::Level min_level = logger::DEBUG;

std::unique_ptr<logger::ILogger>& Log::logger() {
    static std::unique_ptr<logger::ILogger> inst;
    return inst;
}

bool Log::should_log(logger::Level level, logger::CodePoint cp) {
    return level >= min_level && logger() && logger()->should_log(level, cp);
}

void Log::set_max_level(logger::Level val) {
    max_level = val;
}

}
