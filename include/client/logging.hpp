/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef LIBGKFS_LOGGING_HPP
#define LIBGKFS_LOGGING_HPP

#include <libsyscall_intercept_hook_point.h>

#include <type_traits>
#include <boost/optional.hpp>
#include <client/make_array.hpp>
#include <client/syscalls.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <date/tz.h>
#include <hermes.hpp>

#ifdef GKFS_DEBUG_BUILD
#include <bitset>
#endif

namespace gkfs {
namespace log {

enum class log_level : short {
    print_syscalls       = 1 << 0,
    print_syscalls_entry = 1 << 1,
    print_info           = 1 << 2,
    print_critical       = 1 << 3,
    print_errors         = 1 << 4,
    print_warnings       = 1 << 5,
    print_hermes         = 1 << 6,
    print_mercury        = 1 << 7,
    print_debug          = 1 << 8,

    // for internal use
    print_none           = 0,
    print_all            = print_syscalls | print_syscalls_entry | print_info |
                           print_critical | print_errors | print_warnings |
                           print_hermes | print_mercury | print_debug,
    print_most           = print_all & ~print_syscalls_entry,
    print_help           = 1 << 10
};

inline constexpr log_level
operator&(log_level l1, log_level l2) { 
    return log_level(static_cast<short>(l1) & 
                     static_cast<short>(l2)); 
}

inline constexpr log_level
operator|(log_level l1, log_level l2) { 
    return log_level(static_cast<short>(l1) | 
                     static_cast<short>(l2)); 
}

inline constexpr log_level
operator^(log_level l1, log_level l2) { 
    return log_level(static_cast<short>(l1) ^ 
                     static_cast<short>(l2)); 
}

inline constexpr log_level
operator~(log_level l1) { 
    return log_level(~static_cast<short>(l1)); 
}

inline constexpr bool 
operator!(log_level dm) {
  return static_cast<short>(dm) == 0;
}

inline const log_level&
operator|=(log_level& l1, log_level l2) { 
    return l1 = l1 | l2; 
}

inline const log_level&
operator&=(log_level& l1, log_level l2) { 
    return l1 = l1 & l2; 
}

inline const log_level&
operator^=(log_level& l1, log_level l2) { 
    return l1 = l1 ^ l2; 
}


static const auto constexpr syscall          = log_level::print_syscalls;
static const auto constexpr syscall_at_entry = log_level::print_syscalls_entry;
static const auto constexpr info             = log_level::print_info;
static const auto constexpr critical         = log_level::print_critical;
static const auto constexpr error            = log_level::print_errors;
static const auto constexpr warning          = log_level::print_warnings;
static const auto constexpr hermes           = log_level::print_hermes;
static const auto constexpr mercury          = log_level::print_mercury;
static const auto constexpr debug            = log_level::print_debug;
static const auto constexpr none             = log_level::print_none;
static const auto constexpr most             = log_level::print_most;
static const auto constexpr all              = log_level::print_all;
static const auto constexpr help             = log_level::print_help;

static const auto constexpr level_names = 
    utils::make_array(
        "syscall",
        "syscall", // sycall_entry uses the same name as syscall
        "info",
        "critical",
        "error",
        "warning",
        "hermes",
        "mercury",
        "debug"
);

inline constexpr auto
lookup_level_name(log_level l) {

    assert(l != log::none && l != log::help);

    // since all log levels are powers of 2, we can find a name 
    // very efficiently by counting the number of trailing 0-bits in l
    const auto i = __builtin_ctz(static_cast<short>(l));
    assert(i >= 0 && static_cast<std::size_t>(i) < level_names.size());

    return level_names.at(i);
}


// forward declaration
struct logger;

namespace detail {

template <typename Buffer>
static inline void
log_buffer(std::FILE* fp, 
           Buffer&& buffer) {
    log_buffer(::fileno(fp), std::forward<Buffer>(buffer));
}

template <typename Buffer>
static inline void
log_buffer(int fd, 
           Buffer&& buffer) {

    if(fd < 0) {
        throw std::runtime_error("Invalid file descriptor");
    }

    ::syscall_no_intercept(SYS_write, fd, buffer.data(), buffer.size());
}

static inline void
log_buffer(int fd,
           const void* buffer,
           std::size_t length) {
    if(fd < 0) {
        throw std::runtime_error("Invalid file descriptor");
    }

    ::syscall_no_intercept(SYS_write, fd, buffer, length);
}

/**
 * format_timestamp_to - safely format a timestamp for logging messages
 * 
 * This function produes a timestamp that can be used to prefix logging 
 * messages. Since we are actively intercepting system calls, the formatting
 * MUST NOT rely on internal system calls, otherwise we risk recursively 
 * calling ourselves for each syscall generated. Also, we cannot rely on 
 * the C formatting functions asctime, ctime, gmtime, localtime, mktime, 
 * asctime_r, ctime_r, gmtime_r, localtime_r, since they acquire a 
 * non-reentrant lock to determine the caller's timezone (yes, the assumedly 
 * reentrant *_r versions of the functions exhibit this problem as well,
 * see https://sourceware.org/bugzilla/show_bug.cgi?id=16145). To solve this
 * issue and still get readable timestamps, we determine and cache the 
 * timezone when the logger is created so that the lock is only held once, by
 * one thread exactly, and we pass it as an argument whenever we need to 
 * format a timestamp. If no timezone is provided, we just format the epoch.
 *
 * NOTE: we use the date C++ library to query the timezone database and
 * to format the timestamps.
 */
template <typename Buffer>
static inline void
format_timestamp_to(Buffer&& buffer,
                    const date::time_zone * const timezone = nullptr) {

    struct ::timeval tv;

    int rv = ::syscall_no_intercept(SYS_gettimeofday, &tv, NULL);

    if(::syscall_error_code(rv) != 0) {
        return;
    }

    date::sys_time<std::chrono::microseconds> now{
        std::chrono::seconds{tv.tv_sec} + 
        std::chrono::microseconds{tv.tv_usec}};

    if(!timezone) {
        fmt::format_to(buffer, "[{}] ", now.time_since_epoch().count());
        return;
    }

    fmt::format_to(buffer, "[{}] ",
            date::zoned_time<std::chrono::microseconds>{timezone, now});
}

template <typename Buffer>
static inline void
format_syscall_info_to(Buffer&& buffer,
                       gkfs::syscall::info info) {

    const auto ttid = syscall_no_intercept(SYS_gettid);
    fmt::format_to(buffer, "[{}] [syscall] ", ttid);

    char o;
    char t;

    switch(gkfs::syscall::origin(info)) {
        case gkfs::syscall::from_internal_code:
            o = 'i';
            break;
        case gkfs::syscall::from_external_code:
            o = 'a';
            break;
        default:
            o = '?';
            break;
    }

    switch(gkfs::syscall::target(info)) {
        case gkfs::syscall::to_hook:
            t = 'h';
            break;
        case gkfs::syscall::to_kernel:
            t = 'k';
            break;
        default:
            t = '?';
            break;
    }

    const std::array<char, 5> tmp = {'[', o, t, ']', ' '};
    fmt::format_to(buffer, fmt::string_view(tmp.data(), tmp.size()));
}

} // namespace detail

enum { max_buffer_size = LIBGKFS_LOG_MESSAGE_SIZE };

struct static_buffer : public fmt::basic_memory_buffer<char, max_buffer_size> {

protected:
    void grow(std::size_t size) override final;
};


struct logger {

    logger(const std::string& opts, 
           const std::string& path, 
           bool trunc
#ifdef GKFS_DEBUG_BUILD
           ,
           const std::string& filter,
           int verbosity
#endif
           );

    ~logger();

    template <typename... Args>
    inline void
    log(log_level level, 
        const char * const func,
        const int lineno,
        Args&&... args) {

        if(!(level & log_mask_)) {
            return;
        }

        static_buffer buffer;
        detail::format_timestamp_to(buffer, timezone_);
        fmt::format_to(buffer, "[{}] [{}] ", 
                ::syscall_no_intercept(SYS_gettid),
                lookup_level_name(level));

        if(!!(level & log::debug)) {
            fmt::format_to(buffer, "<{}():{}> ", func, lineno);
        }

        fmt::format_to(buffer, std::forward<Args>(args)...);
        fmt::format_to(buffer, "\n");
        detail::log_buffer(log_fd_, buffer);
    }

    inline int 
    log(log_level level, 
        const char *fmt, 
        va_list ap) {

        if(!(level & log_mask_)) {
            return 0;
        }

        // we use buffer views to compose the logging messages to 
        // avoid copying buffers as much as possible
        struct buffer_view {
            const void* addr;
            std::size_t size;
        };

        // helper lambda to print an iterable of buffer_views
        const auto log_buffer_views = 
            [this](const auto& buffers) {

                std::size_t n = 0;

                for(const auto& bv : buffers) {
                    if(bv.addr != nullptr) {
                        detail::log_buffer(log_fd_, bv.addr, bv.size);
                        n += bv.size;
                    }
                }

                return n;
            };
             


        static_buffer prefix;
        detail::format_timestamp_to(prefix);
        fmt::format_to(prefix, "[{}] [{}] ", 
                ::syscall_no_intercept(SYS_gettid),
                lookup_level_name(level));

        char buffer[max_buffer_size];
        const int n = vsnprintf(buffer, sizeof(buffer), fmt, ap);

        std::array<buffer_view, 3> buffers{};

        int i = 0;
        int m = 0;
        const char* addr = buffer;
        const char* p = nullptr;
        while((p = std::strstr(addr, "\n")) != nullptr)  {
            buffers[0] = buffer_view{prefix.data(), prefix.size()};
            buffers[1] = buffer_view{addr, static_cast<std::size_t>(p - addr) + 1};

            m += log_buffer_views(buffers);
            addr = p + 1;
            ++i;
        }

        // original line might not end with (or include) '\n'
        if(buffer[n-1] != '\n') {
            buffers[0] = buffer_view{prefix.data(), prefix.size()};
            buffers[1] = buffer_view{addr, static_cast<std::size_t>(&buffer[n] - addr)};
            buffers[2] = buffer_view{"\n", 1};

            m += log_buffer_views(buffers);
        }

        return m;
    }

    template <typename... Args>
    static inline void
    log_message(std::FILE* fp, Args&&... args) {
        log_message(::fileno(fp), std::forward<Args>(args)...);
    }

    template <typename... Args>
    static inline void
    log_message(int fd, Args&&... args) {

        if(fd < 0) {
            throw std::runtime_error("Invalid file descriptor");
        }

        static_buffer buffer;
        fmt::format_to(buffer, std::forward<Args>(args)...);
        fmt::format_to(buffer, "\n");
        detail::log_buffer(fd, buffer);
    }

    void
    log_syscall(syscall::info info,
                const long syscall_number, 
                const long args[6],
                boost::optional<long> result = boost::none);

    static std::shared_ptr<logger>& global_logger() {
        static std::shared_ptr<logger> s_global_logger;
        return s_global_logger;
    }

    int log_fd_;
    log_level log_mask_;

#ifdef GKFS_DEBUG_BUILD
    std::bitset<512> filtered_syscalls_;
    int debug_verbosity_;
#endif

    const date::time_zone * timezone_;
};

// the following static functions can be used to interact 
// with a globally registered logger instance

template <typename... Args>
static inline void 
create_global_logger(Args&&... args) {

    auto foo = std::make_shared<logger>(std::forward<Args>(args)...);
    logger::global_logger() = foo;
        
}

static inline void 
register_global_logger(logger&& lg) {
    logger::global_logger() = std::make_shared<logger>(std::move(lg));
}

static inline std::shared_ptr<logger>& 
get_global_logger() {
    return logger::global_logger();
}

static inline void 
destroy_global_logger() {
    logger::global_logger().reset();
}

inline void
static_buffer::grow(std::size_t size) {

    const auto logger = get_global_logger();

    if(logger) {
        logger->log_mask_ &= ~(syscall | syscall_at_entry);
    }

    std::fprintf(stderr, 
"FATAL: message too long for gkfs::log::static_buffer, increase the size of\n" 
"LIBGKFS_LOG_MESSAGE_SIZE in CMake or reduce the length of the offending "
"message.\n");
    abort();
}

} // namespace log
} // namespace gkfs

#define LOG(XXX, ...) LOG_##XXX(__VA_ARGS__)

#ifndef GKFS_ENABLE_LOGGING

#define LOG_INFO(...) do {} while(0);
#define LOG_WARNING(...) do {} while(0);
#define LOG_ERROR(...) do {} while(0);
#define LOG_CRITICAL(...) do {} while(0);
#define LOG_HERMES(...)  do {} while(0);
#define LOG_MERCURY(...) do {} while(0);
#define LOG_SYSCALL(...) do {} while(0);
#define LOG_DEBUG(...) do {} while(0);

#else // !GKFS_ENABLE_LOGGING

#define LOG_INFO(...) do {                                              \
    if(gkfs::log::get_global_logger()) {                                \
        gkfs::log::get_global_logger()->log(                            \
                gkfs::log::info, __func__, __LINE__, __VA_ARGS__);      \
    }                                                                   \
} while(0);

#define LOG_WARNING(...) do {                                           \
    if(gkfs::log::get_global_logger()) {                                \
        gkfs::log::get_global_logger()->log(                            \
                gkfs::log::warning, __func__, __LINE__, __VA_ARGS__);   \
    }                                                                   \
} while(0);

#define LOG_ERROR(...) do {                                             \
    if(gkfs::log::get_global_logger()) {                                \
        gkfs::log::get_global_logger()->log(                            \
                gkfs::log::error, __func__, __LINE__, __VA_ARGS__);     \
    }                                                                   \
} while(0);

#define LOG_CRITICAL(...) do {                                          \
    if(gkfs::log::get_global_logger()) {                                \
        gkfs::log::get_global_logger()->log(                            \
                gkfs::log::critical, __func__, __LINE__, __VA_ARGS__);  \
    }                                                                   \
} while(0);

#define LOG_HERMES(...) do { \
    if(gkfs::log::get_global_logger()) {                                \
        gkfs::log::get_global_logger()->log(                            \
                gkfs::log::hermes, __func__, __LINE__, __VA_ARGS__);    \
    }                                                                   \
} while(0);

#define LOG_MERCURY(...) do { \
    if(gkfs::log::get_global_logger()) {                                \
        gkfs::log::get_global_logger()->log(                            \
                gkfs::log::mercury, __func__, __LINE__, __VA_ARGS__);   \
    }                                                                   \
} while(0);

#ifdef GKFS_DEBUG_BUILD

#define LOG_SYSCALL(...) do {                                           \
if(gkfs::log::get_global_logger()) {                                    \
        gkfs::log::get_global_logger()->log_syscall(__VA_ARGS__);       \
    }                                                                   \
} while(0);

#define LOG_DEBUG(...) do {                                             \
    if(gkfs::log::get_global_logger()) {                                \
        gkfs::log::get_global_logger()->log(                            \
                gkfs::log::debug, __func__, __LINE__, __VA_ARGS__);     \
    }                                                                   \
} while(0);

#else // ! GKFS_DEBUG_BUILD

#define LOG_SYSCALL(...) do {} while(0);
#define LOG_DEBUG(...) do {} while(0);

#endif // ! GKFS_DEBUG_BUILD
#endif // !GKFS_ENABLE_LOGGING

#endif // LIBGKFS_LOGGING_HPP
