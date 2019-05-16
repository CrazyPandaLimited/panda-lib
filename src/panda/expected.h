#pragma once
#include <tl/expected.hpp>

namespace panda {
    template <class T, class E>
    using expected = tl::expected<T,E>;
}
