/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef LIBGKFS_UTILS_MAKE_ARRAY_HPP
#define LIBGKFS_UTILS_MAKE_ARRAY_HPP

namespace gkfs {
namespace utils {

template <typename... T>
constexpr auto make_array(T&&... values) ->
    std::array<
       typename std::decay<
           typename std::common_type<T...>::type>::type,
       sizeof...(T)> {
    return std::array<
        typename std::decay<
            typename std::common_type<T...>::type>::type,
        sizeof...(T)>{std::forward<T>(values)...};
}

} // namespace utils
} // namespace gkfs

#endif // LIBGKFS_UTILS_MAKE_ARRAY_HPP
