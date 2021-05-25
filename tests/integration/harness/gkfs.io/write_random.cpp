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
#include <random>
#include <climits>

/* C includes */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif
constexpr int seed = 42;

using json = nlohmann::json;

struct write_random_options {
    bool verbose{};
    std::string pathname{};
    ::size_t count{};

    REFL_DECL_STRUCT(write_random_options,
                     REFL_DECL_MEMBER(bool, verbose),
                     REFL_DECL_MEMBER(std::string, pathname),
                     REFL_DECL_MEMBER(::size_t, count)
    );
};

struct write_random_output {
    ::ssize_t retval;
    int errnum;

    REFL_DECL_STRUCT(write_random_output,
                     REFL_DECL_MEMBER(::size_t, retval),
                     REFL_DECL_MEMBER(int, errnum)
    );
};

void to_json(json& record,
             const write_random_output& out) {
    record = serialize(out);
}

/**
 * Writes `count` random bytes to file
 * @param opts
 */
void write_random_exec(const write_random_options& opts) {

    int fd = ::open(opts.pathname.c_str(), O_WRONLY);

    if (fd == -1) {
        if (opts.verbose) {
            fmt::print("open(pathname=\"{}\", count={}) = {}, errno: {} [{}]\n",
                       opts.pathname, opts.count, fd, errno, ::strerror(errno));
            return;
        }

        json out = write_random_output{fd, errno};
        fmt::print("{}\n", out.dump(2));
        return;
    }
    // random number generator with seed
    std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char> engine{seed};
    // create buffer for opts.count
    std::vector<uint8_t> data(opts.count);
    std::generate(begin(data), end(data), std::ref(engine));
    // pass data to buffer
    io::buffer buf(data);

    int rv = ::write(fd, buf.data(), opts.count);

    if (opts.verbose) {
        fmt::print("write(pathname=\"{}\", count={}) = {}, errno: {} [{}]\n",
                   opts.pathname, opts.count, rv, errno, ::strerror(errno));
        return;
    }

    json out = write_random_output{rv, errno};
    fmt::print("{}\n", out.dump(2));
}

void write_random_init(CLI::App& app) {

    // Create the option and subcommand objects
    auto opts = std::make_shared<write_random_options>();
    auto* cmd = app.add_subcommand(
            "write_random",
            "Execute the write() system call ");

    // Add options to cmd, binding them to opts
    cmd->add_flag(
            "-v,--verbose",
            opts->verbose,
            "Produce human writeable output"
    );

    cmd->add_option(
                    "pathname",
                    opts->pathname,
                    "File name"
            )
            ->required()
            ->type_name("");

    cmd->add_option(
                    "count",
                    opts->count,
                    "Number of random bytes to write"
            )
            ->required()
            ->type_name("");

    cmd->callback([opts]() {
        write_random_exec(*opts);
    });
}