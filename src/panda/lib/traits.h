#pragma once

#include <utility>

namespace panda {
namespace lib {
namespace traits {

template<typename T, bool Trivial = std::is_class<T>::value>
struct is_comparable {
    static const bool value = true;
};

template<typename T>
struct is_comparable<T, true> {
    struct fallback { bool operator==(const fallback& oth); };
    struct mixed_type: std::remove_reference<T>::type, fallback {};
    template < typename U, U > struct type_check {};

    template < typename U > static std::false_type  test( type_check< bool (fallback::*)(const fallback&), &U::operator== >* = 0 );
    template < typename U > static std::true_type   test( ... );

    static const bool value = std::is_same<decltype(test<mixed_type>(nullptr)), std::true_type>::value;
};


template<typename T, typename... Args>
struct has_call_operator {
private:
    typedef std::true_type yes;
    typedef std::false_type no;

    template<typename U> static auto test(int) -> decltype(std::declval<U>()(std::declval<Args>()...), yes());
    template<typename> static no test(...);

public:
    static constexpr bool value = std::is_same<decltype(test<T>(0)),yes>::value;
};

}
}
}
