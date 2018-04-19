#include "log.h"
#include <iomanip>

namespace panda {

static logger::Level min_level = logger::DEBUG;

std::unique_ptr<logger::ILogger>& Log::logger() {
    static std::unique_ptr<logger::ILogger> inst;
    return inst;
}

bool Log::should_log(logger::Level level, logger::CodePoint cp) {
    return level >= min_level && logger() && logger()->should_log(level, cp);
}

void Log::set_level(logger::Level val) {
    min_level = val;
}

std::ostream&logger::operator <<(std::ostream& stream, const logger::escaped& str) {
   for (auto c : str.src) {
       if (isprint(c)) {
           stream << c;
       } else {
           stream << "\\" << std::setfill('0') << std::setw(3) << uint32_t(uint8_t(c));
       }
   }
   return stream;
}

}
