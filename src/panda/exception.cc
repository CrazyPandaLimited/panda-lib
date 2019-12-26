#include "exception.h"
#include <cstring>
#include <memory>
#include <functional>

#if defined(__unix__)
  #include <execinfo.h>
#endif

namespace panda {

static backtrace_producer* producer = nullptr;

void backtrace::install_producer(backtrace_producer& producer_) {
    producer = &producer_;
}

backtrace::backtrace (const backtrace& other) noexcept : buffer(other.buffer) {}
	
#if defined(__unix__)

backtrace::backtrace () noexcept {
    buffer.resize(max_depth);
    auto depth = ::backtrace(buffer.data(), max_depth);
    buffer.resize(depth);
}

backtrace::~backtrace() {}

iptr<backtrace_info> backtrace::get_backtrace_info() const noexcept {
    if (producer) { return (*producer)(buffer); }
    return iptr<backtrace_info>();
}

#else
  
backtrace::backtrace () noexcept {}
string backtrace::get_trace_string () const { return {}; }

#endif


exception::exception () noexcept {}

exception::exception (const string& whats) noexcept : _whats(whats) {}

exception::exception (const exception& oth) noexcept : backtrace(oth), _whats(oth._whats) {}

exception& exception::operator= (const exception& oth) noexcept {
    _whats = oth._whats;
    backtrace::operator=(oth);
    return *this;
}

const char* exception::what () const noexcept {
    _whats = whats();
    return _whats.c_str();
}

string exception::whats () const noexcept {
    return _whats;
}

}
