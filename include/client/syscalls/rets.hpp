/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GKFS_SYSCALLS_RETS_HPP
#define GKFS_SYSCALLS_RETS_HPP

#include <fmt/format.h>
#include <type_traits>
#include <client/syscalls/detail/syscall_info.h>

namespace gkfs {
namespace syscall {
namespace ret {

/** Allowed ret types (based on the values of the corresponding C enum) */
enum class type {
    none        = ::ret_type_t::rnone,
    ptr         = ::ret_type_t::rptr,
    dec         = ::ret_type_t::rdec,
};

/* Some constant definitions for convenience */
static constexpr auto none        = type::none;
static constexpr auto ptr         = type::ptr;
static constexpr auto dec         = type::dec;


/** All ret formatters must follow this prototype */
template <typename FmtBuffer>
using formatter = 
    std::add_pointer_t<void(FmtBuffer&, long)>;


/** forward declare formatters */
template <typename FmtBuffer> inline void
format_none_ret_to(FmtBuffer& buffer, long val);

template <typename FmtBuffer> inline void
format_ptr_ret_to(FmtBuffer& buffer, long val);

template <typename FmtBuffer> inline void
format_dec_ret_to(FmtBuffer& buffer, long val);

/** Known formatters */
template <typename Buffer>
static const constexpr 
std::array<formatter<Buffer>, ret_type_max> formatters = {
    /* [rnone]       = */ format_none_ret_to,
    /* [rptr]        = */ format_ptr_ret_to,
    /* [rdec]        = */ format_dec_ret_to,
};

/** A return value descriptor */
struct desc {
    ret::type type_;

    ret::type
    type() const {
        return type_;
    }

    bool
    operator==(ret::type t) const {
        return type_ == t;
    }

    bool
    operator!=(ret::type t) const {
        return type_ != t;
    }

    template <typename FmtBuffer>
    formatter<FmtBuffer>
    formatter() const {
        const auto idx = static_cast<int>(type_);

        // if the type is unknown fall back to the default formatter
        if(idx < 0 || idx >= static_cast<int>(formatters<FmtBuffer>.size())) {
            return format_dec_ret_to;
        }

        assert(formatters<FmtBuffer>.at(idx) != nullptr);

        return formatters<FmtBuffer>.at(idx);
    }
};


/** Specific formatter implementations follow */
#define LIKELY(x)      __builtin_expect(!!(x), 1)
#define UNLIKELY(x)    __builtin_expect(!!(x), 0)

template <typename FmtBuffer>
inline void
format_none_ret_to(FmtBuffer& buffer,
                   long val) {
    fmt::format_to(buffer, "void");
}

template <typename FmtBuffer>
inline void
format_ptr_ret_to(FmtBuffer& buffer,
                  long val) {
    if(LIKELY(reinterpret_cast<const void*>(val) != nullptr)) {
        fmt::format_to(buffer, "{}", reinterpret_cast<const void*>(val));
        return;
    }

    fmt::format_to(buffer, "NULL");
}

template <typename FmtBuffer>
inline void
format_dec_ret_to(FmtBuffer& buffer,
                  long val) {
    fmt::format_to(buffer, "{}", val);
}

#undef LIKELY
#undef UNLIKELY

} // namespace ret
} // namespace syscall
} // namespace gkfs

#endif // GKFS_SYSCALLS_RETS_HPP
