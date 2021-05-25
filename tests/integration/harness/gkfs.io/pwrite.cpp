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

struct pwrite_options {
    bool verbose;
    std::string pathname;
    std::string data;
    ::size_t count;
    ::size_t offset;

    REFL_DECL_STRUCT(pwrite_options,
        REFL_DECL_MEMBER(bool, verbose),
        REFL_DECL_MEMBER(std::string, pathname),
        REFL_DECL_MEMBER(std::string, data),
        REFL_DECL_MEMBER(::size_t, count),
        REFL_DECL_MEMBER(::size_t, offset)
    );
};

struct pwrite_output {
    ::ssize_t retval;
    int errnum;

    REFL_DECL_STRUCT(pwrite_output,
        REFL_DECL_MEMBER(::size_t, retval),
        REFL_DECL_MEMBER(int, errnum)
    );
};

void
to_json(json& record, 
        const pwrite_output& out) {
    record = serialize(out);
}

void 
pwrite_exec(const pwrite_options& opts) {

    int fd = ::open(opts.pathname.c_str(), O_WRONLY);

    if(fd == -1) {
        if(opts.verbose) {
            fmt::print("pwrite(pathname=\"{}\", buf=\"{}\" count={}, offset={}) = {}, errno: {} [{}]\n", 
                    opts.pathname, opts.data, opts.count, opts.offset, fd, errno, ::strerror(errno));
            return;
        }

        json out = pwrite_output{fd, errno};
        fmt::print("{}\n", out.dump(2));

        return;
    }

    io::buffer buf(opts.data);
    int rv = ::pwrite(fd, buf.data(), opts.count, opts.offset);

    if(opts.verbose) {
        fmt::print("pwrite(pathname=\"{}\", count={}, offset={}) = {}, errno: {} [{}]\n", 
                   opts.pathname, opts.count, opts.offset, rv, errno, ::strerror(errno));
        return;
    }

    json out = pwrite_output{rv, errno};
    fmt::print("{}\n", out.dump(2));
}

void
pwrite_init(CLI::App& app) {

    // Create the option and subcommand objects
    auto opts = std::make_shared<pwrite_options>();
    auto* cmd = app.add_subcommand(
            "pwrite", 
            "Execute the pwrite() system call");

    // Add options to cmd, binding them to opts
    cmd->add_flag(
            "-v,--verbose",
            opts->verbose,
            "Produce human writeable output"
        );

    cmd->add_option(
            "pathname", 
            opts->pathname,
            "Directory name"
        )
        ->required()
        ->type_name("");

    cmd->add_option(
            "data", 
            opts->data,
            "Data to write"
        )
        ->required()
        ->type_name("");

    cmd->add_option(
            "count", 
            opts->count,
            "Number of bytes to write"
        )
        ->required()
        ->type_name("");

    cmd->add_option(
            "offset", 
            opts->offset,
            "Offset to read"
        )
        ->required()
        ->type_name("");

    cmd->callback([opts]() { 
        pwrite_exec(*opts); 
    });
}


