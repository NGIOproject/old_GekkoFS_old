/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GKFS_IO_REFLECTION_HPP
#define GKFS_IO_REFLECTION_HPP

#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <dirent.h> // required by DIR*

#include <boost/preprocessor.hpp>


namespace refl {
namespace detail {

template<typename Class, typename T>
struct property_impl {
    constexpr property_impl(T Class::*aMember, 
                            const char* aType, 
                            const char* aName) : 
        member{aMember}, 
        type{aType}, 
        name{aName} {}

    using Type = T;

    T Class::*member;
    const char* type;
    const char* name;
};

} // namespace detail

template<typename Class, typename T>
constexpr auto 
property(T Class::*member, const char* type, const char* name) {
    return detail::property_impl<Class, T>{member, type, name};
}

template <typename T, T... S, typename F>
constexpr void for_sequence(std::integer_sequence<T, S...>, F&& f) {
    using unpack_t = int[];
    (void) unpack_t{(static_cast<void>(f(std::integral_constant<T, S>{})), 0)..., 0};
}

} // namespace refl


/* private helper macros, do not call directly */
#define _REFL_STRUCT_NAME(t) BOOST_PP_TUPLE_ELEM(0, t)
#define _REFL_STRUCT_MEMBER_COUNT(t) BOOST_PP_TUPLE_ELEM(1, t)
#define _REFL_MEMBER_TYPE(t) BOOST_PP_TUPLE_ELEM(0, t)
#define _REFL_MEMBER_NAME(t) BOOST_PP_TUPLE_ELEM(1, t)

#define _REFL_GEN_FIELD(r, data, index, elem) \
    refl::property( \
        &_REFL_STRUCT_NAME(data)::_REFL_MEMBER_NAME(elem), \
        BOOST_PP_STRINGIZE(_REFL_MEMBER_TYPE(elem)), \
        BOOST_PP_STRINGIZE(_REFL_MEMBER_NAME(elem)) \
    ) \
    BOOST_PP_COMMA_IF( \
        BOOST_PP_NOT_EQUAL(index, \
            BOOST_PP_DEC(_REFL_STRUCT_MEMBER_COUNT(data))))

/* public interface */
#define REFL_DECL_MEMBER(type, name) \
    (type, name)

#define REFL_DECL_STRUCT(struct_name, ...) \
    constexpr static auto properties = std::make_tuple( \
        BOOST_PP_SEQ_FOR_EACH_I( \
            _REFL_GEN_FIELD, \
            ( struct_name, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__ ) ), \
            BOOST_PP_TUPLE_TO_SEQ( ( __VA_ARGS__ ) )) \
    );\
    static_assert(true, "")

#endif // GKFS_IO_REFLECTION_HPP
