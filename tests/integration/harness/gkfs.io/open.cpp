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
#include <binary_buffer.hpp>

/* C includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using json = nlohmann::json;

struct open_options {
    bool verbose;
    std::string pathname;
    int flags;
    ::mode_t mode;

    REFL_DECL_STRUCT(open_options,
        REFL_DECL_MEMBER(bool, verbose),
        REFL_DECL_MEMBER(std::string, pathname),
        REFL_DECL_MEMBER(int, flags),
        REFL_DECL_MEMBER(::mode_t, mode)
    );
};

struct open_output {
    int retval;
    int errnum;

    REFL_DECL_STRUCT(open_output,
        REFL_DECL_MEMBER(int, retval),
        REFL_DECL_MEMBER(int, errnum)
    );
};

void
to_json(json& record, 
        const open_output& out) {
    record = serialize(out);
}

void 
open_exec(const open_options& opts) {

    int fd = ::open(opts.pathname.c_str(), opts.flags, opts.mode);

    if(opts.verbose) {
        fmt::print("open(pathname=\"{}\", flags={}, mode={:#04o}) = {}, errno: {} [{}]\n", 
                opts.pathname, opts.flags, opts.mode, fd, errno, ::strerror(errno));
        return;
    }

    json out = open_output{fd, errno};
    fmt::print("{}\n", out.dump(2));

    return;
}

void
open_init(CLI::App& app) {

    // Create the option and subcommand objects
    auto opts = std::make_shared<open_options>();
    auto* cmd = app.add_subcommand(
            "open", 
            "Execute the open() system call");

    // Add options to cmd, binding them to opts
    cmd->add_flag(
            "-v,--verbose",
            opts->verbose,
            "Produce human readable output"
        );

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
            "Open flags"
        )
        ->required()
        ->type_name("")
        ->check(CLI::NonNegativeNumber);

    cmd->add_option(
            "mode", 
            opts->mode,
            "Octal mode specified for the new directory (e.g. 0664)"
        )
        //->required()
        ->default_val(0)
        ->type_name("")
        ->check(CLI::NonNegativeNumber);


    cmd->callback([opts]() { 
        open_exec(*opts); 
    });
}


