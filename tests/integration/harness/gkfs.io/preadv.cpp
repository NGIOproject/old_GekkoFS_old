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
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>

using json = nlohmann::json;

struct preadv_options {
    bool verbose;
    std::string pathname;
    ::size_t count_0;
    ::size_t count_1;
    ::size_t offset;

    REFL_DECL_STRUCT(preadv_options,
        REFL_DECL_MEMBER(bool, verbose),
        REFL_DECL_MEMBER(std::string, pathname),
        REFL_DECL_MEMBER(::size_t, count_0),
        REFL_DECL_MEMBER(::size_t, count_1),
        REFL_DECL_MEMBER(::size_t, offset)
    );
};

struct preadv_output {
    ::ssize_t retval;
    io::buffer buf_0;
    io::buffer buf_1;
    int errnum;

    REFL_DECL_STRUCT(preadv_output,
        REFL_DECL_MEMBER(::size_t, retval),
        REFL_DECL_MEMBER(void*, buf_0),
        REFL_DECL_MEMBER(void*, buf_1),
        REFL_DECL_MEMBER(int, errnum)
    );
};

void
to_json(json& record, 
        const preadv_output& out) {
    record = serialize(out);
}

void 
preadv_exec(const preadv_options& opts) {

    int fd = ::open(opts.pathname.c_str(), O_RDONLY);

    if(fd == -1) {
        if(opts.verbose) {
            fmt::print("preadv(pathname=\"{}\", count_0={}, count_1={}, offset={}) = {}, errno: {} [{}]\n", 
                    opts.pathname, opts.count_0, opts.count_1, opts.offset, fd, errno, ::strerror(errno));
            return;
        }

        json out = preadv_output{fd, nullptr, nullptr, errno};
        fmt::print("{}\n", out.dump(2));

        return;
    }

    io::buffer buf_0(opts.count_0);
    io::buffer buf_1(opts.count_1);

    struct iovec iov[2];

    iov[0].iov_base = buf_0.data();
    iov[1].iov_base = buf_1.data();

    iov[0].iov_len = opts.count_0;
    iov[1].iov_len = opts.count_1;

    int rv = ::preadv(fd, iov, 2, opts.offset);

    if(opts.verbose) {
        fmt::print("preadv(pathname=\"{}\", count_0={}, count_1={}, offset={}) = {}, errno: {} [{}]\n", 
                   opts.pathname, opts.count_0, opts.count_1, opts.offset, rv, errno, ::strerror(errno));
        return;
    }

    json out = preadv_output{rv, (rv != -1 ? buf_0 : nullptr), (rv != -1 ? buf_1 : nullptr), errno};
    fmt::print("{}\n", out.dump(2));
}

void
preadv_init(CLI::App& app) {

    // Create the option and subcommand objects
    auto opts = std::make_shared<preadv_options>();
    auto* cmd = app.add_subcommand(
            "preadv", 
            "Execute the preadv() system call");

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
            "count_0", 
            opts->count_0,
            "Number of bytes to read to buffer 0"
        )
        ->required()
        ->type_name("");

    cmd->add_option(
            "count_1", 
            opts->count_1,
            "Number of bytes to read to buffer 1"
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
        preadv_exec(*opts); 
    });
}

