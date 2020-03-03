#include "error.h"
#include <iostream>

namespace panda {

namespace error {
    static thread_local std::map<std::pair<const std::error_category*, const NestedCategory*>, NestedCategory> _cache;

    const NestedCategory& get_nested_category(const std::error_category& self, const NestedCategory* next) {
        auto& cache = _cache;
        auto iter = cache.find({&self, next});
        if (iter != cache.end()) {
            return iter->second;
        } else {
            return cache.emplace(std::piecewise_construct,
                                 std::forward_as_tuple(&self, next),
                                 std::forward_as_tuple(self, next)).first->second;
        }
    }

    const NestedCategory& std_system_category = get_nested_category(std::system_category(), nullptr);
}

ErrorCode ErrorCode::next () const noexcept {
    if (codes.size() <= 1) return {};
    CodeStack new_stack = codes;
    new_stack.pop();
    const error::NestedCategory* new_cat = cat->next;
    return ErrorCode(std::move(new_stack), new_cat);
}

string ErrorCode::what () const {
    if (!value()) return {};

    string ret(codes.size() * 50);

    int i = 0;
    ErrorCode ec = *this;
    while (ec) {
        if (i) ret += " -> ";
        ret += ec.message().c_str();
        ret += " (";
        ret += to_string(ec.value());
        ret += ":";
        ret += ec.category().name();
        ret += ")";
        ec = ec.next();
        ++i;
    }

    return ret;
}

std::ostream& operator<< (std::ostream& os, const ErrorCode& ec) {
    return os << ec.what();
}

}
