#pragma once
#include <tl/expected.hpp>

namespace panda {
    template <class T, class E>
    using expected = tl::expected<T,E>;

    template <class E>
    using unexpected = tl::unexpected<E>;

    template<class E>
    using bad_expected_access = tl::bad_expected_access<E>;

    using unexpect_t = tl::unexpect_t;

    inline constexpr unexpect_t unexpect{};
}
