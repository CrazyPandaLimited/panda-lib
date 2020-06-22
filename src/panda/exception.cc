#include "exception.h"
#include <cstring>
#include <memory>
#include <functional>
#ifdef _WIN32
  #include "exception/win.icc"
#else
  #include "exception/unix.icc"
#endif

namespace panda {

static RawTraceProducer  rawtrace_producer = get_default_raw_producer();
static BacktraceProducer bt_producer       = get_default_bt_producer();

BacktraceInfo::~BacktraceInfo() {};

string BacktraceInfo::to_string() const noexcept {
    string r;
    for(auto& frame: frames) {
        // mimic gdb-style
        r += "0x";
        r += string::from_number(frame->address, 16);
        if (frame->name) {
            r += " in ";
            r += frame->name;
        }
        if (frame->file) {
            r += " at ";
            r += frame->file;
            if (frame->line_no) {
                r += ":";
                r += string::from_number(frame->line_no, 10);
            }
        } else if(frame->library) {
            r += " from ";
            r += frame->library;
        }
        r += "\n";
    }
    return r;
}

void Backtrace::install_producer(BacktraceProducer& producer_) {
    bt_producer = producer_;
}

Backtrace::Backtrace (const Backtrace& other) noexcept : buffer(other.buffer) {}
	
Backtrace::Backtrace () noexcept {
    void* temp_buff[max_depth];
    auto depth = rawtrace_producer(temp_buff, max_depth);
    if (depth > 0) {
        buffer.resize(max_depth);
        std::memcpy(buffer.data(), temp_buff, sizeof(void*) * depth);
    }
}

Backtrace::~Backtrace() {}

iptr<BacktraceInfo> Backtrace::get_backtrace_info() const noexcept {
    return bt_producer(buffer);
}

string Backtrace::dump_trace() noexcept {
    return Backtrace().get_backtrace_info()->to_string();
}

exception::exception () noexcept {}

exception::exception (const string& whats) noexcept : _whats(whats) {}

exception::exception (const exception& oth) noexcept : Backtrace(oth), _whats(oth._whats) {}

exception& exception::operator= (const exception& oth) noexcept {
    _whats = oth._whats;
    Backtrace::operator=(oth);
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
