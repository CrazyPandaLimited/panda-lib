#include "log.h"

panda::Log::Dispatcher& panda::Log::loggers() {
    static Dispatcher inst;
    return inst;
}
