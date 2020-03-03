#include "error.h"
#include <iostream>

namespace panda {

namespace error {
    static thread_local std::map<std::pair<const std::error_category*, const NestedCategory*>, NestedCategory> _cache;

    const NestedCategory& get_nested_categoty(const std::error_category& self, const NestedCategory* next) {
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
}

std::ostream& operator<< (std::ostream& os, const ErrorCode& ec) {
    return os << ec.what();
}

}
