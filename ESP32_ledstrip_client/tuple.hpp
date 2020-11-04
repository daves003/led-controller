#pragma once

//////////////// enable_if ////////////////
template<bool cond, typename T = void>
struct enable_if { };

template<typename T>
struct enable_if<true, T> { using type = T; };

template<bool cond, typename T = void>
using enable_if_t = typename enable_if<cond, T>::type;


////////////////// tuple //////////////////
namespace detail
{
    template<unsigned I, typename T>
    struct tuple_leaf {
        constexpr tuple_leaf(T&& t) : value(static_cast<T&&>(t)) {}
        T value;
    };

    template<unsigned I, typename... Ts>
    struct tuple_impl;

    template<unsigned I>
    struct tuple_impl<I>{};

    template<unsigned I, typename T, typename... Ts>
    struct tuple_impl<I, T, Ts...> :
        public tuple_leaf<I, T>, // This adds a `value` member of type HeadItem
        public tuple_impl<I + 1, Ts...> // This recurses
    {
        constexpr tuple_impl(T&& t, Ts&& ...ts)
            : tuple_leaf<I, T>(static_cast<T&&>(t))
            , tuple_impl<I+1, Ts...>(static_cast<Ts&&>(ts)...)
        {}
    };

    template<unsigned I, typename T, typename... Ts>
    constexpr T& get(tuple_impl<I, T, Ts...>& tuple)
    {
        return tuple.tuple_leaf<I, T>::value;
    }
    template<unsigned I, typename T, typename... Ts>
    constexpr const T& get(const tuple_impl<I, T, Ts...>& tuple)
    {
        return tuple.tuple_leaf<I, T>::value;
    }
}

template<typename... Ts>
using tuple = detail::tuple_impl<0, Ts...>;
using detail::get;



///////////////// for_each ////////////////
namespace detail
{
    template<unsigned I, typename Function_t, typename... Tp>
    constexpr
    enable_if_t<(I == sizeof...(Tp))>
    for_each_impl(const tuple<Tp...>&, Function_t)
    {}

    template<unsigned I, typename Function_t, typename... Tp>
    constexpr
    enable_if_t<(I < sizeof...(Tp))>
    for_each_impl(const tuple<Tp...>& tuple, Function_t function)
    {
        function(get<I>(tuple));
        for_each_impl<I+1, Function_t, Tp...>(tuple, function);
    }
}

template<typename Function_t, typename... Tp>
constexpr void for_each(const tuple<Tp...>& tuple, Function_t function)
{
    detail::for_each_impl<0, Function_t, Tp...>(tuple, function);
}
