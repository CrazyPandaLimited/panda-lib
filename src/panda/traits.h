#pragma once

namespace panda {

namespace detail {
    template <bool...> struct bool_pack {};
}

template <bool...T> using conjunction = std::is_same<detail::bool_pack<true,T...>, detail::bool_pack<T..., true>>;
template <bool...T> using disjunction = std::integral_constant<bool, !std::is_same<detail::bool_pack<false,T...>, detail::bool_pack<T..., false>>::value>;

template <class T, class...Args> using is_one_of           = disjunction<std::is_same<T,Args>::value...>;
template <class T, class...Args> using enable_if_one_of_t  = std::enable_if_t<is_one_of<T,Args...>::value, T>;
template <class T, class...Args> using enable_if_one_of_vt = std::enable_if_t<is_one_of<T,Args...>::value, void>;

template <class T, class R = T> using enable_if_arithmetic_t        = std::enable_if_t<std::is_arithmetic<T>::value, R>;
template <class T, class R = T> using enable_if_signed_integral_t   = std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value, R>;
template <class T, class R = T> using enable_if_unsigned_integral_t = std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value, R>;
template <class T, class R = T> using enable_if_floatp_t            = std::enable_if_t<std::is_floating_point<T>::value, R>;

}
