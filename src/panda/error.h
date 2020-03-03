#pragma once
#include "memory.h"
#include "string.h"
#include "varint.h"
#include <map>
#include <stack>
#include <iosfwd>
#include <system_error>

namespace panda {

namespace error {
    struct NestedCategory : std::error_category {
        const std::error_category& self;
        const NestedCategory* next;

        constexpr NestedCategory(const std::error_category& self, const NestedCategory* next = nullptr) noexcept : self(self), next(next) {}

        // delegate all implementation to self
        virtual const char* name() const noexcept { return self.name(); }
        virtual std::error_condition default_error_condition(int code) const noexcept { return self.default_error_condition(code); }
        virtual bool equivalent(int code, const std::error_condition& condition) const noexcept {
            return self.equivalent(code, condition);
        }
        virtual bool equivalent(const std::error_code& code, int condition) const noexcept {
            return self.equivalent(code, condition);
        }
        virtual std::string message(int condition) const { return self.message(condition); }
        bool operator==( const std::error_category& rhs ) const noexcept { return self.operator ==(rhs); }
        bool operator!=( const std::error_category& rhs ) const noexcept { return self.operator !=(rhs); }
        bool operator<( const std::error_category& rhs ) const noexcept  { return self.operator <(rhs); }
    };

    const NestedCategory& get_nested_category(const std::error_category& self, const NestedCategory* next);

    extern const NestedCategory& std_system_category;
}

struct ErrorCode : AllocatedObject<ErrorCode> {
    ErrorCode() noexcept {
        codes.push(0);
        cat = &error::std_system_category;
    }

    ErrorCode(const ErrorCode& o) = default;
    ErrorCode(ErrorCode&&) = default;

    ErrorCode(int ec, const std::error_category& ecat) noexcept
        : cat(&error::get_nested_category(ecat, nullptr))
    {
        codes.push(ec);
    }

    ErrorCode(const std::error_code& ec) noexcept {
        if (!ec) {
            codes.push(0);
            cat = &error::std_system_category;
        } else {
            codes.push(ec.value());
            cat = &error::get_nested_category(ec.category(), nullptr);
        }
    }

    template <class ErrorCodeEnum, typename = std::enable_if_t<std::is_error_code_enum<ErrorCodeEnum>::value, void>>
    ErrorCode (ErrorCodeEnum e) noexcept : ErrorCode(std::error_code(e)) {}

    ErrorCode(const std::error_code& c, const ErrorCode& next) noexcept
        : codes(next.codes)
        , cat(&error::get_nested_category(c.category(), next.cat))
    {
        codes.push(c.value());
    }

    ErrorCode& operator=(const ErrorCode& o) noexcept {
        codes = o.codes;
        cat = o.cat;
        return *this;
    }

    ErrorCode& operator=(ErrorCode&& o) noexcept {
        codes = std::move(o.codes);
        cat = o.cat;
        return *this;
    }

    template <class ErrorCodeEnum, typename = std::enable_if_t<std::is_error_code_enum<ErrorCodeEnum>::value, void>>
    ErrorCode& operator=(ErrorCodeEnum e) noexcept {
        std::error_code ec(e);
        codes = CodeStack{};
        codes.push(ec.value());
        cat = &error::get_nested_category(ec.category(), nullptr);
        return *this;
    }

    void assign( int ec, const std::error_category& ecat ) noexcept {
        codes = CodeStack{};
        codes.push(ec);
        cat = &error::get_nested_category(ecat, nullptr);
    }

    void clear() noexcept {
        *this = {};
    }

    int value() const noexcept {
        return codes.top();
    }

    const std::error_category& category() const noexcept {
        return cat->self;
    }

    std::error_condition default_error_condition() const noexcept {
        return code().default_error_condition();
    }

    std::string message() const {
        return cat->message(codes.top());
    }

    string what () const;

    explicit operator bool() const noexcept {
        return value();
    }

    std::error_code code() const noexcept {
        return std::error_code(codes.top(), cat->self);
    }

    ErrorCode next () const noexcept;

    ~ErrorCode() = default;

    // any user can add specialization for his own result and get any data
    template <typename T = void, typename... Args>
    T private_access(Args...);

    template <typename T = void, typename... Args>
    T private_access(Args...) const;

private:
    using CodeStack = VarIntStack;

    ErrorCode(CodeStack&& codes, const error::NestedCategory* cat) : codes(std::move(codes)), cat(cat) {}

    CodeStack codes;
    const error::NestedCategory* cat;
};

inline bool operator==(const ErrorCode& lhs, const ErrorCode& rhs) noexcept { return lhs.code() == rhs.code(); }
//inline bool operator==(const ErrorCode& lhs, const std::error_code& rhs) noexcept { return lhs.code() == rhs; }
//inline bool operator==(const std::error_code& lhs, const ErrorCode& rhs) noexcept { return lhs == rhs.code(); }

inline bool operator!=(const ErrorCode& lhs, const ErrorCode& rhs) noexcept { return !(lhs.code() == rhs.code()); }
//inline bool operator!=(const ErrorCode& lhs, const std::error_code& rhs) noexcept { return lhs.code() != rhs; }
//inline bool operator!=(const std::error_code& lhs, const ErrorCode& rhs) noexcept { return lhs != rhs.code(); }

inline bool operator<(const ErrorCode& lhs, const ErrorCode& rhs) noexcept { return lhs.code() < rhs.code(); }
//inline bool operator<(const ErrorCode& lhs, const std::error_code& rhs) noexcept { return lhs.code() < rhs; }
//inline bool operator<(const std::error_code& lhs, const ErrorCode& rhs) noexcept { return lhs < rhs.code(); }

std::ostream& operator<< (std::ostream&, const ErrorCode&);

}

namespace std {
    template<> struct hash<panda::ErrorCode> {
        typedef panda::ErrorCode argument_type;
        typedef std::size_t result_type;

        result_type operator()(argument_type const& c) const noexcept {
            result_type const h1 ( std::hash<std::error_code>{}(c.code()) );
            result_type const h2 ( std::hash<size_t>{}((size_t)&c.category()));
            return h1 ^ (h2 << 1); // simplest hash combine
        }
    };
}
