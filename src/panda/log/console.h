#pragma once
#include "../log.h"

namespace panda { namespace log {

struct ConsoleLogger : ILogger {
    ConsoleLogger () {}

    void log (Level, const CodePoint&, const string&) override;
};

}}
