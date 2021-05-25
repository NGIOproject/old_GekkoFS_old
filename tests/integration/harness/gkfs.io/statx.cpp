/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

/* C++ includes */
#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <fmt/format.h>
#include <commands.hpp>
#include <reflection.hpp>
#include <serialize.hpp>

/* C includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>           /* Definition of AT_* constants */

using json = nlohmann::json;

/*   int statx(int dirfd, const char *pathname, int flags,
                 unsigned int mask, struct statx *statxbuf);
*/

#ifdef STATX_TYPE
struct statx_options {
    bool verbose;
    int dirfd;
    std::string pathname;
    int flags;
    unsigned int mask;

    REFL_DECL_STRUCT(statx_options,
        REFL_DECL_MEMBER(bool, verbose),
        REFL_DECL_MEMBER(int, dirfd),
        REFL_DECL_MEMBER(std::string, pathname),
        REFL_DECL_MEMBER(int, flags),
        REFL_DECL_MEMBER(unsigned int,  mask)
    );
};

struct statx_output {
    int retval;
    int errnum;
    struct ::statx statbuf;

    REFL_DECL_STRUCT(statx_output,
        REFL_DECL_MEMBER(int, retval),
        REFL_DECL_MEMBER(int, errnum),
        REFL_DECL_MEMBER(struct ::statx, statbuf)
    );
};

void
to_json(json& record, 
        const statx_output& out) {
    record = serialize(out);
}

void 
statx_exec(const statx_options& opts) {

    struct ::statx statbuf;

    int rv = ::statx(opts.dirfd, opts.pathname.c_str(), opts.flags, opts.mask, &statbuf);

    if(opts.verbose) {
        fmt::print("statx(dirfd={}, pathname=\"{}\", flags={}, mask={}) = {}, errno: {} [{}]\n", 
                   opts.dirfd, opts.pathname, opts.flags, opts.mask, rv, errno, ::strerror(errno));
        return;
    }

    json out = statx_output{rv, errno, statbuf};
    fmt::print("{}\n", out.dump(2));
}

void
statx_init(CLI::App& app) {

    // Create the option and subcommand objects
    auto opts = std::make_shared<statx_options>();
    auto* cmd = app.add_subcommand(
            "statx", 
            "Execute the statx() system call");

    // Add options to cmd, binding them to opts
    cmd->add_flag(
            "-v,--verbose",
            opts->verbose,
            "Produce human readable output"
        );

    cmd->add_option(
            "dirfd", 
            opts->dirfd,
            "file descritor"
        )
        ->required()
        ->type_name("");

    cmd->add_option(
            "pathname", 
            opts->pathname,
            "Directory name"
        )
        ->required()
        ->type_name("");

    cmd->add_option(
            "flags",
            opts->flags,
            "Flags"
        )
        ->required()
        ->type_name("");

    cmd->add_option(
            "mask",
            opts->mask,
            "Mask"
        )
        ->required()
        ->type_name("");

    cmd->callback([opts]() { 
        statx_exec(*opts); 
    });
}
#endif
