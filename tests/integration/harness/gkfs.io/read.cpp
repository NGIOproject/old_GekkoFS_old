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

struct read_options {
    bool verbose;
    std::string pathname;
    ::size_t count;

    REFL_DECL_STRUCT(read_options,
        REFL_DECL_MEMBER(bool, verbose),
        REFL_DECL_MEMBER(std::string, pathname),
        REFL_DECL_MEMBER(::size_t, count)
    );
};

struct read_output {
    ::ssize_t retval;
    io::buffer buf;
    int errnum;

    REFL_DECL_STRUCT(read_output,
        REFL_DECL_MEMBER(::size_t, retval),
        REFL_DECL_MEMBER(void*, buf),
        REFL_DECL_MEMBER(int, errnum)
    );
};

void
to_json(json& record, 
        const read_output& out) {
    record = serialize(out);
}

void 
read_exec(const read_options& opts) {

    int fd = ::open(opts.pathname.c_str(), O_RDONLY);

    if(fd == -1) {
        if(opts.verbose) {
            fmt::print("read(pathname=\"{}\", count={}) = {}, errno: {} [{}]\n", 
                    opts.pathname, opts.count, fd, errno, ::strerror(errno));
            return;
        }

        json out = read_output{fd, nullptr, errno};
        fmt::print("{}\n", out.dump(2));

        return;
    }

    io::buffer buf(opts.count);

    int rv = ::read(fd, buf.data(), opts.count);

    if(opts.verbose) {
        fmt::print("read(pathname=\"{}\", count={}) = {}, errno: {} [{}]\n", 
                   opts.pathname, opts.count, rv, errno, ::strerror(errno));
        return;
    }

    json out = read_output{rv, (rv != -1 ? buf : nullptr), errno};
    fmt::print("{}\n", out.dump(2));
}

void
read_init(CLI::App& app) {

    // Create the option and subcommand objects
    auto opts = std::make_shared<read_options>();
    auto* cmd = app.add_subcommand(
            "read", 
            "Execute the read() system call");

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
            "count", 
            opts->count,
            "Number of bytes to read"
        )
        ->required()
        ->type_name("");

    cmd->callback([opts]() { 
        read_exec(*opts); 
    });
}

