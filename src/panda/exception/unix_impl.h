#pragma once
#include "../exception.h"

#if defined(__unix__) || defined(__APPLE__)

namespace panda { namespace impl {

RawTraceProducer get_default_raw_producer() noexcept;
BacktraceProducer get_default_bt_producer() noexcept;

}}

#endif
