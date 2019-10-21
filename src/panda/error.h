#pragma once

#include <system_error>

#include <panda/memory.h>
#include <panda/string.h>

namespace panda {

struct ErrorCode : AllocatedObject<ErrorCode> {
    ErrorCode() noexcept : _code(), _next(nullptr) {}
    ErrorCode(const ErrorCode& o) : _code(o._code), _next(o._next ? new ErrorCode(*o._next) : nullptr) {}
    ErrorCode(ErrorCode&&) = default;

    ErrorCode(int ec, const std::error_category& ecat) noexcept : _code(ec, ecat), _next(nullptr) {}

    template< class ErrorCodeEnum >
    explicit ErrorCode(ErrorCodeEnum e) noexcept : _code(e), _next(nullptr) {}

    explicit ErrorCode(const std::error_code& c) noexcept : _code(c), _next(nullptr) {}

    ErrorCode(const std::error_code& c, const ErrorCode& next) noexcept : _code(c), _next(new ErrorCode(next)) {}

    ErrorCode& operator=(const ErrorCode& o) noexcept {
        _code = o._code;
        _next.reset(o._next ? new ErrorCode(*o._next) : nullptr);
        return *this;
    }

    ErrorCode& operator=(ErrorCode&& o) noexcept = default;

    template <class ErrorCodeEnum>
    ErrorCode& operator=(ErrorCodeEnum e ) noexcept {
        _code = e;
        _next.reset();
        return *this;
    }

    void assign( int ec, const std::error_category& ecat ) noexcept {
        _code.assign(ec, ecat);
        _next.reset();
    }

    void clear() noexcept {
        _code.clear();
        _next.reset();
    }

    int value() const noexcept {
        return _code.value();
    }

    const std::error_category& category() const noexcept {
        return _code.category();
    }

    std::error_condition default_error_condition() const noexcept {
        return _code.default_error_condition();
    }

    string message() const {
        // mau be optimized by getting all messages from list on stack and one allocation for whole message
        std::string std_msg(_code.message());
        string res(std_msg.data(), std_msg.length());
        if (_next) {
            res += ", preceded by:\n" + _next->message();
        }
        return res;
    }

    explicit operator bool() const noexcept {
        return _code.operator bool();
    }

    const std::error_code& code() const noexcept {
        return _code;
    }

    const ErrorCode* next() const noexcept {
        return _next.get();
    }

    ~ErrorCode() = default;

private:
    std::error_code _code;
    std::unique_ptr<ErrorCode> _next;
};

inline bool operator==( const ErrorCode& lhs, const std::error_code& rhs ) noexcept { return lhs.code() == rhs; }
inline bool operator==( const std::error_code& lhs, const ErrorCode& rhs ) noexcept { return lhs == rhs.code(); }

inline bool operator!=( const ErrorCode& lhs, const std::error_code& rhs ) noexcept { return lhs.code() != rhs; }
inline bool operator!=( const std::error_code& lhs, const ErrorCode& rhs ) noexcept { return lhs != rhs.code(); }

inline bool operator<( const ErrorCode& lhs, const std::error_code& rhs ) noexcept { return lhs.code() < rhs; }
inline bool operator<( const std::error_code& lhs, const ErrorCode& rhs ) noexcept { return lhs < rhs.code(); }

template< class CharT, class Traits >
std::basic_ostream<CharT,Traits>& operator<<( std::basic_ostream<CharT,Traits>& os, const ErrorCode& ec ) {
    return os << ec.message();
}

}

namespace std {
template<> struct hash<panda::ErrorCode> {
    typedef panda::ErrorCode argument_type;
    typedef std::size_t result_type;

    result_type operator()(argument_type const& c) const noexcept {
        result_type const h1 ( std::hash<std::error_code>{}(c.code()) );
        result_type const h2 ( std::hash<size_t>{}((size_t)c.next()) );
        return h1 ^ (h2 << 1); // simplest hash combine
    }
};
}
