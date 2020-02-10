/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GKFS_SYSCALLS_ARGS_HPP
#define GKFS_SYSCALLS_ARGS_HPP

#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syscall.h>
#include <unistd.h>

#include <type_traits>
#include <fmt/format.h>
#include <client/syscalls/detail/syscall_info.h>

namespace gkfs {
namespace syscall {
namespace arg {

/** Allowed arg types (based on the values of the corresponding C enum) */
enum class type {
    none          = ::arg_type_t::none,
    fd            = ::arg_type_t::fd,
    atfd          = ::arg_type_t::atfd,
    cstr          = ::arg_type_t::cstr,
    open_flags    = ::arg_type_t::open_flags,
    octal_mode    = ::arg_type_t::octal_mode,
    ptr           = ::arg_type_t::ptr,
    dec           = ::arg_type_t::dec,
    dec32         = ::arg_type_t::dec32,
    offset        = ::arg_type_t::offset,
    whence        = ::arg_type_t::whence,
    mmap_prot     = ::arg_type_t::mmap_prot,
    mmap_flags    = ::arg_type_t::mmap_flags,
    clone_flags   = ::arg_type_t::clone_flags,
    signum        = ::arg_type_t::signum,
    sigproc_how   = ::arg_type_t::sigproc_how,
    generic       = ::arg_type_t::arg,
};

/* Some constant definitions for convenience */
static constexpr auto none          = type::none;
static constexpr auto fd            = type::fd;
static constexpr auto atfd          = type::atfd;
static constexpr auto cstr          = type::cstr;
static constexpr auto open_flags    = type::open_flags;
static constexpr auto octal_mode    = type::octal_mode;
static constexpr auto ptr           = type::ptr;
static constexpr auto dec           = type::dec;
static constexpr auto dec32         = type::dec32;
static constexpr auto offset        = type::offset;
static constexpr auto whence        = type::whence;
static constexpr auto mmap_prot     = type::mmap_prot;
static constexpr auto mmap_flags    = type::mmap_flags;
static constexpr auto clone_flags   = type::clone_flags;
static constexpr auto signum        = type::signum;
static constexpr auto sigproc_how   = type::sigproc_how;
static constexpr auto generic       = type::generic;


/** An argument value with an optional size */
struct printable_arg {
    const char * const name;
    const long value;
    boost::optional<long> size;
};


/** All arg formatters must follow this prototype */
template <typename FmtBuffer>
using formatter = 
    std::add_pointer_t<void(FmtBuffer&, const printable_arg&)>;



/** forward declare formatters */
template <typename FmtBuffer> inline void
format_none_arg_to(FmtBuffer& buffer, const printable_arg& parg);

template <typename FmtBuffer> inline void
format_fd_arg_to(FmtBuffer& buffer, const printable_arg& parg);

template <typename FmtBuffer> inline void
format_atfd_arg_to(FmtBuffer& buffer, const printable_arg& parg);

template <typename FmtBuffer> inline void
format_cstr_arg_to(FmtBuffer& buffer, const printable_arg& parg);

template <typename FmtBuffer> inline void
format_open_flags_to(FmtBuffer& buffer, const printable_arg& parg);

template <typename FmtBuffer> inline void
format_octal_mode_to(FmtBuffer& buffer, const printable_arg& parg);

template <typename FmtBuffer> inline void 
format_ptr_arg_to(FmtBuffer& buffer, const printable_arg& parg);

template <typename FmtBuffer> inline void
format_dec_arg_to(FmtBuffer& buffer, const printable_arg& parg);

template <typename FmtBuffer> inline void
format_dec32_arg_to(FmtBuffer& buffer, const printable_arg& parg);

template <typename FmtBuffer> inline void
format_whence_arg_to(FmtBuffer& buffer, const printable_arg& parg);

template <typename FmtBuffer> inline void
format_mmap_prot_arg_to(FmtBuffer& buffer, const printable_arg& parg);

template <typename FmtBuffer> inline void
format_mmap_flags_arg_to(FmtBuffer& buffer, const printable_arg& parg);

template <typename FmtBuffer> inline void
format_clone_flags_arg_to(FmtBuffer& buffer, const printable_arg& parg);

template <typename FmtBuffer> inline void
format_signum_arg_to(FmtBuffer& buffer, const printable_arg& parg);

template <typename FmtBuffer> inline void
format_sigproc_how_arg_to(FmtBuffer& buffer, const printable_arg& parg);

template <typename FmtBuffer> inline void
format_arg_to(FmtBuffer& buffer, const printable_arg& parg);


/** Known formatters */
template <typename FmtBuffer>
static const constexpr 
std::array<formatter<FmtBuffer>, arg_type_max> formatters = {
    /* [none]          = */ format_none_arg_to,
    /* [fd]            = */ format_fd_arg_to,
    /* [atfd]          = */ format_atfd_arg_to,
    /* [cstr]          = */ format_cstr_arg_to,
    /* [open_flags]    = */ format_open_flags_to,
    /* [octal_mode]    = */ format_octal_mode_to,
    /* [ptr]           = */ format_ptr_arg_to,
    /* [dec]           = */ format_dec_arg_to,
    /* [dec32]         = */ format_dec32_arg_to,
    /* [offset]        = */ format_dec_arg_to,
    /* [whence]        = */ format_whence_arg_to,
    /* [mmap_prot]     = */ format_mmap_prot_arg_to,
    /* [mmap_flags]    = */ format_mmap_flags_arg_to,
    /* [clone_flags]   = */ format_clone_flags_arg_to,
    /* [signum]        = */ format_signum_arg_to,
    /* [sigproc_how]   = */ format_sigproc_how_arg_to,
    /* [arg]           = */ format_arg_to,
};

/** An argument descriptor */
struct desc {
    arg::type   type_;
    const char* name_;

    arg::type
    type() const {
        return type_;
    }

    const char*
    name() const {
        return name_;
    }

    template <typename FmtBuffer>
    formatter<FmtBuffer>
    formatter() const {
        const auto idx = static_cast<int>(type_);

        // if the type is unknown fall back to the default formatter
        if(idx < 0 || idx >= static_cast<int>(formatters<FmtBuffer>.size())) {
            return format_arg_to;
        }

        assert(formatters<FmtBuffer>.at(idx) != nullptr);

        return formatters<FmtBuffer>.at(idx);
    }
};


/** Specific formatter implementations follow */

/** Flag descriptor */
typedef struct {
	long               flag_;
	const char * const name_;
} flag_desc;

#define FLAG_ENTRY(f) flag_desc{ f, #f }

#define LIKELY(x)      __builtin_expect(!!(x), 1)
#define UNLIKELY(x)    __builtin_expect(!!(x), 0)

template <typename FmtBuffer, typename FlagDescriptorArray>
static void
format_flag(FmtBuffer& buffer, 
            long flag,
		    FlagDescriptorArray&& desc) {

    // we assume that if a flag value is zero, its printable
    // name will always be at position 0 in the array
	if(flag == 0 && desc[0].flag_ == 0) {
        fmt::format_to(buffer, "{}", desc[0].name_);
        return;
    }

    for(std::size_t i = 0; i < desc.size(); ++i) {

        if(desc[i].name_ == nullptr) {
            continue;
        }

        if((flag == desc[i].flag_)) {
            fmt::format_to(buffer, "{}", desc[i].name_);
            return;
        }
    }

    fmt::format_to(buffer, "{:#x}", flag);
}

template <typename FmtBuffer, typename FlagDescriptorArray>
static void
format_flag_set(FmtBuffer& buffer, 
                long flags,
		        FlagDescriptorArray&& desc) {

    // we assume that if a flag value is zero, its printable
    // name will always be at position 0 in the array
	if(flags == 0 && desc[0].flag_ == 0) {
        fmt::format_to(buffer, "{}", desc[0].name_);
        return;
    }

    std::size_t i = 0;
	const auto buffer_start = buffer.size();

	while(flags != 0 && i < desc.size()) {

	    if(desc[i].name_ == nullptr) {
            ++i;
	        continue;
        }

        if((flags & desc[i].flag_) != 0) {
            fmt::format_to(buffer, "{}{}", 
                    buffer.size() != buffer_start ? "|" : "",
                    desc[i].name_);
            flags &= ~desc[i].flag_;
        }

        ++i;
    }

	if(flags != 0) {
		if(buffer.size() != buffer_start) {
            fmt::format_to(buffer, "|");
        }

        fmt::format_to(buffer, "{:#x}", flags);
        return;
	}

	if(buffer_start == buffer.size()) {
        fmt::format_to(buffer, "0x0");
    }
}

/** 
 * format_whence_arg_to - format a 'whence' argument
 *
 * Format a 'whence' argument from the lseek() syscall, modifying the provided
 * buffer by appending to it a string representation of the form:
 *   name = formatted_val  
 */
template <typename FmtBuffer> 
inline void
format_whence_arg_to(FmtBuffer& buffer, 
                     const printable_arg& parg) {

    /* Names for lseek() whence arg */
    const auto flag_names = 
        utils::make_array(
            FLAG_ENTRY(SEEK_SET),
            FLAG_ENTRY(SEEK_CUR),
            FLAG_ENTRY(SEEK_END)
        );

    fmt::format_to(buffer, "{}=", parg.name);
    format_flag_set(buffer, parg.value, flag_names);
}


/** 
 * format_mmap_prot_arg_to - format a 'prot' argument
 *
 * Format a 'prot' argument (such as those passed to mmap())
 * and append the resulting string to the provided buffer.
 */
template <typename FmtBuffer> 
inline void
format_mmap_prot_arg_to(FmtBuffer& buffer, 
                        const printable_arg& parg) {

    /* Names for mmap() prot arg */
    const auto flag_names =
        utils::make_array(
            FLAG_ENTRY(PROT_NONE),
            FLAG_ENTRY(PROT_READ),
            FLAG_ENTRY(PROT_WRITE),
            FLAG_ENTRY(PROT_EXEC));

    fmt::format_to(buffer, "{}=", parg.name);
    format_flag_set(buffer, parg.value, flag_names);

    return;
}


/** 
 * format_mmap_flags_arg_to - format a 'flags' argument
 *
 * Format a 'flags' argument (such as those passed to mmap())
 * and append the resulting string to the provided buffer.
 */
template <typename FmtBuffer> 
inline void
format_mmap_flags_arg_to(FmtBuffer& buffer, 
                         const printable_arg& parg) {

    /* Names for mmap() flags arg */
    const auto flag_names =
        utils::make_array(
            FLAG_ENTRY(MAP_SHARED),
            FLAG_ENTRY(MAP_PRIVATE),
#ifdef MAP_SHARED_VALIDATE
            FLAG_ENTRY(MAP_SHARED_VALIDATE),
#endif
            FLAG_ENTRY(MAP_FIXED),
            FLAG_ENTRY(MAP_ANONYMOUS),
            FLAG_ENTRY(MAP_GROWSDOWN),
            FLAG_ENTRY(MAP_DENYWRITE),
            FLAG_ENTRY(MAP_EXECUTABLE),
            FLAG_ENTRY(MAP_LOCKED),
            FLAG_ENTRY(MAP_NORESERVE),
            FLAG_ENTRY(MAP_POPULATE),
            FLAG_ENTRY(MAP_NONBLOCK),
            FLAG_ENTRY(MAP_STACK),
            FLAG_ENTRY(MAP_HUGETLB)
#ifdef MAP_SYNC
            ,
            FLAG_ENTRY(MAP_SYNC)
#endif
            );

    fmt::format_to(buffer, "{}=", parg.name);
    format_flag_set(buffer, parg.value, flag_names);
    return;
}

/** 
 * format_clone_flags_arg_to - format a 'flags' argument
 *
 * Format a 'flags' argument (such as those passed to clone())
 * and append the resulting string to the provided buffer.
 */
template <typename FmtBuffer> 
inline void
format_clone_flags_arg_to(FmtBuffer& buffer, 
                          const printable_arg& parg) {

    /* Names for clone() flags arg */
    const auto flag_names =
        utils::make_array(
            FLAG_ENTRY(CLONE_VM),
            FLAG_ENTRY(CLONE_FS),
            FLAG_ENTRY(CLONE_FILES),
            FLAG_ENTRY(CLONE_SIGHAND),
            FLAG_ENTRY(CLONE_PTRACE),
            FLAG_ENTRY(CLONE_VFORK),
            FLAG_ENTRY(CLONE_PARENT),
            FLAG_ENTRY(CLONE_THREAD),
            FLAG_ENTRY(CLONE_NEWNS),
            FLAG_ENTRY(CLONE_SYSVSEM),
            FLAG_ENTRY(CLONE_SETTLS),
            FLAG_ENTRY(CLONE_PARENT_SETTID),
            FLAG_ENTRY(CLONE_CHILD_CLEARTID),
            FLAG_ENTRY(CLONE_DETACHED),
            FLAG_ENTRY(CLONE_UNTRACED),
            FLAG_ENTRY(CLONE_CHILD_SETTID),
#ifdef CLONE_NEWCGROUP
            FLAG_ENTRY(CLONE_NEWCGROUP),
#endif
            FLAG_ENTRY(CLONE_NEWUTS),
            FLAG_ENTRY(CLONE_NEWIPC),
            FLAG_ENTRY(CLONE_NEWUSER),
            FLAG_ENTRY(CLONE_NEWPID),
            FLAG_ENTRY(CLONE_NEWNET),
            FLAG_ENTRY(CLONE_IO));

    fmt::format_to(buffer, "{}=", parg.name);

    // the low byte in clone flags contains the number of the termination 
    // signal sent to the parent when the child dies
    format_flag_set(buffer, parg.value & ~0x11l, flag_names);

    if((parg.value & 0x11l) != 0) {
        fmt::format_to(buffer, "|", parg.name);
        format_signum_arg_to(buffer, {"", parg.value & 0x11l});
    }
    return;
}

/** 
 * format_signum_arg_to - format a 'signum' argument
 *
 * Format a 'signum' argument (such as those passed to rt_sigaction())
 * and append the resulting string to the provided buffer.
 */
template <typename FmtBuffer>
inline void
format_signum_arg_to(FmtBuffer& buffer, 
                     const printable_arg& parg) {

    /* Names for signum args */
    const auto flag_names =
        utils::make_array(
            FLAG_ENTRY(SIGHUP),
            FLAG_ENTRY(SIGINT),
            FLAG_ENTRY(SIGQUIT),
            FLAG_ENTRY(SIGILL),
            FLAG_ENTRY(SIGTRAP),
            FLAG_ENTRY(SIGABRT),
            FLAG_ENTRY(SIGBUS),
            FLAG_ENTRY(SIGFPE),
            FLAG_ENTRY(SIGKILL),
            FLAG_ENTRY(SIGUSR1),
            FLAG_ENTRY(SIGSEGV),
            FLAG_ENTRY(SIGUSR2),
            FLAG_ENTRY(SIGPIPE),
            FLAG_ENTRY(SIGALRM),
            FLAG_ENTRY(SIGTERM),
            FLAG_ENTRY(SIGSTKFLT),
            FLAG_ENTRY(SIGCHLD),
            FLAG_ENTRY(SIGCONT),
            FLAG_ENTRY(SIGSTOP),
            FLAG_ENTRY(SIGTSTP),
            FLAG_ENTRY(SIGTTIN),
            FLAG_ENTRY(SIGTTOU),
            FLAG_ENTRY(SIGURG),
            FLAG_ENTRY(SIGXCPU),
            FLAG_ENTRY(SIGXFSZ),
            FLAG_ENTRY(SIGVTALRM),
            FLAG_ENTRY(SIGPROF),
            FLAG_ENTRY(SIGWINCH),
            FLAG_ENTRY(SIGIO),
            FLAG_ENTRY(SIGPWR),
            FLAG_ENTRY(SIGSYS));

    if(std::strcmp(parg.name, "")) {
        fmt::format_to(buffer, "{}=", parg.name);
    }

    format_flag(buffer, parg.value, flag_names);
    return;
}


/** 
 * format_sigproc_how_arg_to - format a 'sigproc how' argument
 *
 * Format a 'sigproc how' argument (such as those passed to sigprocmask())
 * and append the resulting string to the provided buffer.
 */
template <typename FmtBuffer>
inline void
format_sigproc_how_arg_to(FmtBuffer& buffer, 
                          const printable_arg& parg) {

    /* Names for sigproc how args */
    const auto flag_names =
        utils::make_array(
            FLAG_ENTRY(SIG_BLOCK),
            FLAG_ENTRY(SIG_UNBLOCK),
            FLAG_ENTRY(SIG_SETMASK));

    fmt::format_to(buffer, "{}=", parg.name);
    format_flag(buffer, parg.value, flag_names);
    return;
}

/** 
 * format_none_arg_to - format a 'none' argument
 *
 * Format a 'none' argument and append the resulting "void" string to the
 * provided buffer.
 */
template <typename FmtBuffer>
inline void
format_none_arg_to(FmtBuffer& buffer,
                   const printable_arg& parg) {
    fmt::format_to(buffer, "void");
}


/** 
 * format_fd_arg_to - format a 'fd' argument
 *
 * Format a 'fd' argument (such as those passed to read())
 * and append the resulting string to the provided buffer.
 */
template <typename FmtBuffer>
inline void
format_fd_arg_to(FmtBuffer& buffer,
                 const printable_arg& parg) {
    fmt::format_to(buffer, "{}={}", parg.name, static_cast<int>(parg.value));
}


/** 
 * format_atfd_arg_to - format a 'at_fd' argument
 *
 * Format a 'at_fd' argument (such as those passed to openat())
 * and append the resulting string to the provided buffer.
 */
template <typename FmtBuffer>
inline void
format_atfd_arg_to(FmtBuffer& buffer,
                   const printable_arg& parg) {

    if(static_cast<int>(parg.value) == AT_FDCWD) {
        fmt::format_to(buffer, "{}=AT_FDCWD", parg.name);
        return;
    }

    fmt::format_to(buffer, "{}={}", parg.name, static_cast<int>(parg.value));
}


/** 
 * format_cstr_arg_to - format a 'cstr' argument
 *
 * Format a 'cstr' argument (i.e. a null-terminated C string)
 * and append the resulting string to the provided buffer.
 */
template <typename FmtBuffer>
inline void
format_cstr_arg_to(FmtBuffer& buffer,
                   const printable_arg& parg) {

    if(LIKELY(reinterpret_cast<const char*>(parg.value) != nullptr)) {
        fmt::format_to(buffer, "{}=\"{}\"", parg.name, 
                       reinterpret_cast<const char*>(parg.value));
        return;
    }

    fmt::format_to(buffer, "{}=NULL", parg.name);
}

/** 
 * format_open_flags_to - format a 'flags' argument
 *
 * Format a 'flags' argument (such as those passed to open()) and append 
 * the resulting string to the provided buffer.
 */
template <typename FmtBuffer>
inline void
format_open_flags_to(FmtBuffer& buffer,
                     const printable_arg& parg) {

    /* Names for O_ACCMODE args */
    const auto flag_names =
        utils::make_array(
            FLAG_ENTRY(O_RDONLY),
            FLAG_ENTRY(O_WRONLY),
            FLAG_ENTRY(O_RDWR));

    const auto extra_flag_names =
        utils::make_array(
#ifdef O_EXEC
            FLAG_ENTRY(O_EXEC),
#endif
#ifdef O_SEARCH
            FLAG_ENTRY(O_SEARCH),
#endif
            FLAG_ENTRY(O_APPEND),
            FLAG_ENTRY(O_CLOEXEC),
            FLAG_ENTRY(O_CREAT),
            FLAG_ENTRY(O_DIRECTORY),
            FLAG_ENTRY(O_DSYNC),
            FLAG_ENTRY(O_EXCL),
            FLAG_ENTRY(O_NOCTTY),
            FLAG_ENTRY(O_NOFOLLOW),
            FLAG_ENTRY(O_NONBLOCK),
            FLAG_ENTRY(O_RSYNC),
            FLAG_ENTRY(O_SYNC),
            FLAG_ENTRY(O_TRUNC)
#ifdef O_TTY_INIT
            , FLAG_ENTRY(O_TTY_INIT)
#endif
            );

    long flags = parg.value;

    fmt::format_to(buffer, "{}=", parg.name);
    format_flag(buffer, flags & O_ACCMODE, flag_names);

    flags &= ~O_ACCMODE;

#ifdef O_TMPFILE
    // processing it with the other flags can result in
    // printing O_DIRECTORY when it should not be listed.
    //
    // See O_TMPFILE' definition in fcntl-linux.h :
    //     #define __O_TMPFILE   (020000000 | __O_DIRECTORY)
	if ((flags & O_TMPFILE) == O_TMPFILE) {
        format_flag(buffer, O_TMPFILE, flag_names);
	    flags &= ~O_TMPFILE;
    }
#endif // !O_TMPFILE

    if(flags != 0) {
        fmt::format_to(buffer, "|", parg.name);
        format_flag_set(buffer, flags, extra_flag_names);
    }
}

/** 
 * format_octal_mode_to - format a 'mode' argument
 *
 * Format a 'mode' argument (such as those passed to open()) and append the
 * generated string to the provided buffer.
 */
template <typename FmtBuffer>
inline void
format_octal_mode_to(FmtBuffer& buffer,
                     const printable_arg& parg) {
    fmt::format_to(buffer, "{}={:#04o}", parg.name, parg.value);
}

/** 
 * format_ptr_arg_to - format a 'ptr' argument
 *
 * Format a 'ptr' argument (i.e. a C pointer)
 * and append the resulting string to the provided buffer.
 */
template <typename FmtBuffer>
inline void
format_ptr_arg_to(FmtBuffer& buffer,
                  const printable_arg& parg) {

    if(LIKELY(reinterpret_cast<const void*>(parg.value) != nullptr)) {
        fmt::format_to(buffer, "{}={}", parg.name, 
                       reinterpret_cast<const void*>(parg.value));
        return;
    }

    fmt::format_to(buffer, "{}=NULL", parg.name);
}


/** 
 * format_dec_arg_to - format a 'dec' argument
 *
 * Format a 'dec' argument (i.e. an integer of unknwon size)
 * and append the resulting string to the provided buffer.
 */
template <typename FmtBuffer>
inline void
format_dec_arg_to(FmtBuffer& buffer,
                  const printable_arg& parg) {
    fmt::format_to(buffer, "{}={}", parg.name, parg.value);
}


/** 
 * format_dec32_arg_to - format a 'dec32' argument
 *
 * Format a 'dec32' argument (i.e. a 32-bit integer)
 * and append the resulting string to the provided buffer.
 */
template <typename FmtBuffer>
inline void
format_dec32_arg_to(FmtBuffer& buffer,
                    const printable_arg& parg) {
    fmt::format_to(buffer, "{}={}", parg.name, static_cast<int>(parg.value));
}


/** 
 * format_arg_to - format an arbitrary argument
 *
 * Format an arbitrary argument and append the resulting 
 * string to the provided buffer.
 */
template <typename FmtBuffer>
inline void
format_arg_to(FmtBuffer& buffer,
              const printable_arg& parg) {
    fmt::format_to(buffer, "{}={:#x}", parg.name, parg.value);
}

#undef FLAG_ENTRY
#undef LIKELY
#undef UNLIKELY

} // namespace arg
} // namespace syscall
} // namespace gkfs

#endif // GKFS_SYSCALLS_ARGS_HPP
