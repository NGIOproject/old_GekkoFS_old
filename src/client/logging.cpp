/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <client/logging.hpp>
#include <client/env.hpp>
#include <client/make_array.hpp>
#include <boost/algorithm/string.hpp>
#include <date/tz.h>
#include <fmt/ostream.h>

#ifdef GKFS_ENABLE_LOGGING
#include <hermes/logging.hpp>
#endif

namespace gkfs {
namespace log {

struct opt_info {
    const char name_[32];
    const std::size_t length_;
    const char help_text_[8][64];
    const log_level mask_;
};

#define STR_AND_LEN(strbuf) \
    strbuf, sizeof(strbuf) - 1

static const auto constexpr debug_opts = utils::make_array(

    opt_info{STR_AND_LEN("none"), 
             {"don't print any messages"}, 
             log::none},

#ifdef GKFS_DEBUG_BUILD

    opt_info{STR_AND_LEN("syscalls"), 
             {"Trace system calls: print the name of each system call,",
              "its arguments, and its return value. All system calls are",
              "printed after being executed save for those that may not",
              "return, such as execve() and execve_at()",
              "[ default: off ]"},
            log::syscall},

    opt_info{STR_AND_LEN("syscalls_at_entry"), 
             {"Trace system calls: print the name of each system call",
              "and its arguments. All system calls are printed before ",
              "being executed and therefore their return values are not",
              "available in the log",
              "[ default: off ]"},
             log::syscall_at_entry},

#endif // !GKFS_DEBUG_BUILD

    opt_info{STR_AND_LEN("info"), 
             {"Print information messages",
              "[ default: on  ]"}, 
             log::info},

    opt_info{STR_AND_LEN("critical"), 
             {"Print critical errors",
              "[ default: on  ]"}, 
             log::critical},

    opt_info{STR_AND_LEN("errors"), 
             {"Print errors",
              "[ default: on  ]"}, 
             log::error},

    opt_info{STR_AND_LEN("warnings"), 
             {"Print warnings",
              "[ default: on  ]"}, 
             log::warning},

    opt_info{STR_AND_LEN("hermes"),
             {"Print messages from Hermes (GekkoFS high-level RPC library)",
              "[ default: on ]"},
             log::hermes},

    opt_info{STR_AND_LEN("mercury"), 
             {"Print messages from Mercury (GekkoFS low-level RPC library)",
              "[ default: on ]"},
             log::mercury},

#ifdef GKFS_DEBUG_BUILD

    opt_info{STR_AND_LEN("debug"), 
             {"Print debug messages",
              "[ default: off ]"},
             log::debug},

    opt_info{STR_AND_LEN("most"), 
             {"All previous options except 'syscalls_at_entry' combined."}, 
             log::most },

#endif // !GKFS_DEBUG_BUILD

    opt_info{STR_AND_LEN("all"), 
             {"All previous options combined."},
             log::all },

    opt_info{STR_AND_LEN("help"), 
             {"Print this help message and exit."},
             log::help}
);

static const auto constexpr max_debug_opt_length = 
    sizeof("syscalls_at_entry") - 1;

static const auto constexpr max_help_text_rows =
    sizeof(debug_opts[0].help_text_) / sizeof(debug_opts[0].help_text_[0]);

/**
 * process_log_options -- process the string given as parameter to determine
 *                        which debugging options are enabled and return a 
 *                        log_level describing them
 */
log_level
process_log_options(const std::string gkfs_debug) {

#ifndef GKFS_ENABLE_LOGGING

    (void) gkfs_debug;
    logger::log_message(stdout, "warning: logging options ignored: "
                        "logging support was disabled in this build");
    return log::none;

#endif // ! GKFS_ENABLE_LOGGING

    log_level dm = log::none;

    std::vector<std::string> tokens;

    // skip separating white spaces and commas
    boost::split(tokens, gkfs_debug, boost::is_any_of(" ,"));

    for(const auto& t : tokens) {

        bool is_known = false;

        for(const auto& opt : debug_opts) {

            // none disables any future and previous flags observed
            if(t == "none") {
                return log::none;
            }

            if(t == opt.name_) {
                dm |= opt.mask_;
                is_known = true;
                break;
            }
        }

        if(!is_known) {
            logger::log_message(stdout, "warning: logging option '{}' unknown; "
                                "try {}=help", t, gkfs::env::LOG);
        }
    }

    if(!!(dm & log::help)) {
        logger::log_message(stdout, "Valid options for the {} "
                            "environment variable are:\n", gkfs::env::LOG);


        for(const auto& opt : debug_opts) {
            const auto padding = max_debug_opt_length - opt.length_ + 2;

            logger::log_message(stdout, "  {}{:>{}}{}", opt.name_, "", 
                                padding, opt.help_text_[0]);

            for(auto i = 1lu; i < max_help_text_rows; ++i) {
                if(opt.help_text_[i][0] != 0) {
                    logger::log_message(stdout, "  {:>{}}{}", "", 
                                        max_debug_opt_length + 2, 
                                        opt.help_text_[i]);
                }
            }

            logger::log_message(stdout, "");
        }

        logger::log_message(stdout, "\n"
                            "To direct the logging output into a file "
                            "instead of standard output\n"
                            "a filename can be specified using the "
                            "{} environment variable.", gkfs::env::LOG_OUTPUT);
        ::_exit(0);
    }

    return dm;
}

#ifdef GKFS_DEBUG_BUILD
std::bitset<512>
process_log_filter(const std::string& log_filter) {

    std::bitset<512> filtered_syscalls;
    std::vector<std::string> tokens;

    if(log_filter.empty()) {
        return filtered_syscalls;
    }

    // skip separating white spaces and commas
    boost::split(tokens, log_filter, 
            [](char c) { return c == ' ' || c == ','; });

    for(const auto& t : tokens) {
        const auto sc = syscall::lookup_by_name(t);

        if(std::strcmp(sc.name(), "unknown_syscall") == 0) {
            logger::log_message(stdout, "warning: system call '{}' unknown; "
                                "will not filter", t);
            continue;
        }
        
        filtered_syscalls.set(sc.number());
    }

    return filtered_syscalls;
}
#endif // GKFS_DEBUG_BUILD

logger::logger(const std::string& opts, 
               const std::string& path, 
               bool trunc
#ifdef GKFS_DEBUG_BUILD
               ,
               const std::string& filter,
               int verbosity
#endif
               ) :
    timezone_(nullptr) {

    /* use stderr by default */
    log_fd_ = 2;
    log_mask_ = process_log_options(opts);

#ifdef GKFS_DEBUG_BUILD
    filtered_syscalls_ = process_log_filter(filter);
    debug_verbosity_ = verbosity;
#endif

    if(!path.empty()) {
		int flags = O_CREAT | O_RDWR | O_APPEND | O_TRUNC;

		if(!trunc) {
			flags &= ~O_TRUNC;
        }

        // we use ::open() here rather than ::syscall_no_intercept(SYS_open)
        // because we want the call to be intercepted by our hooks, which 
        // allows us to categorize the resulting fd as 'internal' and
        // relocate it to our private range
		int fd = ::open(path.c_str(), flags, 0600);

		if(fd == -1) {
            log(gkfs::log::error, __func__, __LINE__, "Failed to open log "
                "file '{}'. Logging will fall back to stderr", path);
            return;
        }

        log_fd_ = fd;
    }

    // Finding the current timezone implies accessing OS files (i.e. syscalls),
    // but current_zone() doesn't actually retrieve the time zone but rather
    // provides a descriptor to it that is **atomically initialized** upon its
    // first use. Thus, if we don't force the initialization here, logging the
    // first intercepted syscall will produce a call to date::time_zone::init()
    // (under std::call_once) which internally ends up calling fopen(). Since
    // fopen() ends up calling sys_open(), we will need to generate another
    // timestamp for a system call log entry, which will attempt to call
    // date::time_zone::init() since the prior initialization (under the same
    // std::call_once) has not yet completed.
    //
    // Unfortunately, date::time_zone doesn't provide a function to prevent
    // this lazy initialization, therefore we force it by requesting
    // information from an arbitrary timepoint (January 1st 1970) which forces
    // the initialization. This doesn't do any actual work and could safely be
    // removed if the date API ends up providing this functionality.
    try {
        timezone_ = date::current_zone();
#ifdef GKFS_DEBUG_BUILD
        using namespace date;
        timezone_->get_info(date::sys_days{January/1/1970});
#endif // GKFS_DEBUG_BUILD
    }
    catch(const std::exception& ex) {
        // if timezone initialization fails, setting timezone_ to nullptr
        // makes format_timestamp_to() default to producing epoch timestamps
        timezone_ = nullptr;
    }

#ifdef GKFS_ENABLE_LOGGING
    const auto log_hermes_message = 
        [](const std::string& msg, hermes::log::level l, int severity, 
           const std::string& file, const std::string& func, int lineno) {

        const auto name = [](hermes::log::level l, int severity) {
            using namespace std::string_literals;

            switch(l) {
                case hermes::log::info:
                    return "info"s;
                case hermes::log::warning:
                    return "warning"s;
                case hermes::log::error:
                    return "error"s;
                case hermes::log::fatal:
                    return "fatal"s;
                case hermes::log::mercury:
                    return "mercury"s;
                default:
                    return "unknown"s;
            }
        };

        LOG(HERMES, "[{}] {}", name(l, severity), msg);
    };

#ifdef GKFS_DEBUG_BUILD
    const auto log_hermes_debug_message = 
        [this](const std::string& msg, hermes::log::level l, 
               int severity, const std::string& file, 
               const std::string& func, int lineno) {

        if(severity > debug_verbosity_) {
            return;
        }

        LOG(HERMES, "[debug{}] <{}():{}> {}", 
                (severity == 0 ? "" : std::to_string(severity + 1)), 
                func, lineno, msg);
    };
#endif // GKFS_DEBUG_BUILD

    const auto log_hg_message = 
        [](const std::string& msg, hermes::log::level l, int severity, 
           const std::string& file, const std::string& func, int lineno) {

        (void) l;

        // mercury message might contain one or more sub-messages 
        // separated by '\n'
        std::vector<std::string> sub_msgs;
        boost::split(sub_msgs, msg, boost::is_any_of("\n"), boost::token_compress_on);

        for(const auto& m : sub_msgs) {
            if(!m.empty()) {
                LOG(MERCURY, "{}", m);
            }
        }
    };

    // register log callbacks into hermes so that we can manage 
    // both its and mercury's log messages 
    hermes::log::logger::register_callback(
            hermes::log::info, log_hermes_message);
    hermes::log::logger::register_callback(
            hermes::log::warning, log_hermes_message);
    hermes::log::logger::register_callback(
            hermes::log::error, log_hermes_message);
    hermes::log::logger::register_callback(
            hermes::log::fatal, log_hermes_message);
#ifdef GKFS_DEBUG_BUILD
    hermes::log::logger::register_callback(
            hermes::log::debug, log_hermes_debug_message);
#endif
    hermes::log::logger::register_callback(
            hermes::log::mercury, log_hg_message);

#endif // GKFS_ENABLE_LOGGING
}

logger::~logger() {
    log_fd_ = ::syscall_no_intercept(SYS_close, log_fd_);
}

void
logger::log_syscall(syscall::info info,
                    const long syscall_number, 
                    const long args[6],
                    boost::optional<long> result) {


    const bool log_syscall_entry = !!(log::syscall_at_entry & log_mask_);
    const bool log_syscall_result = !!(log::syscall & log_mask_);

    // log the syscall if and only if logging for syscalls is enabled
    if(!log_syscall_entry && !log_syscall_result) {
        return;
    }

#ifdef GKFS_DEBUG_BUILD
    if(filtered_syscalls_[syscall_number]) {
        return;
    }
#endif

    // log the syscall even if we don't have information on it, since it may
    // be important to the user (we assume that the syscall has completed 
    // though)
    if(info == syscall::no_info) {
        goto print_syscall;
    }

    // log the syscall entry if the syscall may not return (e.g. execve) or
    // if we are sure that it won't ever return (e.g. exit), even if 
    // log::syscall_at_entry is disabled
    if(syscall::may_not_return(syscall_number) || 
       syscall::never_returns(syscall_number)) {
        goto print_syscall;
    }

    if(log_syscall_entry && syscall::execution_is_pending(info)) {
        goto print_syscall;
    }

    if(log_syscall_result && !syscall::execution_is_pending(info)) {
        goto print_syscall;
    }

    return;

print_syscall:

    static_buffer buffer;

    detail::format_timestamp_to(buffer, timezone_);
    detail::format_syscall_info_to(buffer, info);

    if(result) {
        syscall::decode(buffer, syscall_number, args, *result);
    }
    else {
        syscall::decode(buffer, syscall_number, args);
    }

    fmt::format_to(buffer, "\n");

	::syscall_no_intercept(SYS_write, log_fd_, buffer.data(), buffer.size());
}

} // namespace log
} // namespace gkfs

