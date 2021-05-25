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
#include <binary_buffer.hpp>
#include <memory>
#include <fmt/format.h>
#include <reflection.hpp>
#include <serialize.hpp>

/* C includes */
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

using json = nlohmann::json;

struct file_compare_options {
    bool verbose{};
    std::string path_1{};
    std::string path_2{};
    size_t count{};

    REFL_DECL_STRUCT(file_compare_options,
                     REFL_DECL_MEMBER(bool, verbose),
                     REFL_DECL_MEMBER(std::string, path_1),
                     REFL_DECL_MEMBER(std::string, path_2),
                     REFL_DECL_MEMBER(size_t, count)
    );
};

struct file_compare_output {
    int retval;
    int errnum;

    REFL_DECL_STRUCT(file_compare_output,
                     REFL_DECL_MEMBER(int, retval),
                     REFL_DECL_MEMBER(int, errnum)
    );
};

void
to_json(json& record,
        const file_compare_output& out) {
    record = serialize(out);
}

int open_file(const std::string& path, bool verbose) {
    auto fd = ::open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        if (verbose) {
            fmt::print("open(pathname=\"{}\") = {}, errno: {} [{}]\n",
                       path, fd, errno, ::strerror(errno));
            return -1;
        }
        json out = file_compare_output{fd, errno};
        fmt::print("{}\n", out.dump(2));
        return -1;
    }
    return fd;
}

size_t read_file(io::buffer& buf, int fd, size_t count) {
    ssize_t rv{};
    size_t total{};
    do {
        rv = ::read(fd, buf.data(), count - total);
        total += rv;
    } while (rv > 0 && total < count);

    if (rv < 0 && total != count) {
        json out = file_compare_output{(int) rv, errno};
        fmt::print("{}\n", out.dump(2));
        return 0;
    }
    return total;
}

void
file_compare_exec(const file_compare_options& opts) {

    // Open both files
    auto fd_1 = open_file(opts.path_1, opts.verbose);
    if (fd_1 == -1) {
        return;
    }
    auto fd_2 = open_file(opts.path_2, opts.verbose);
    if (fd_2 == -1) {
        return;
    }

    // read both files
    io::buffer buf_1(opts.count);
    auto rv = read_file(buf_1, fd_1, opts.count);
    if (rv == 0)
        return;

    io::buffer buf_2(opts.count);
    rv = read_file(buf_2, fd_2, opts.count);
    if (rv == 0)
        return;

    // memcmp both files to check if they're equal
    auto comp_rv = memcmp(buf_1.data(), buf_2.data(), opts.count);
    if (comp_rv != 0) {
        if (opts.verbose) {
            fmt::print("memcmp(path_1='{}', path_2='{}', count='{}') = '{}'\n",
                       opts.path_1, opts.path_2, opts.count, comp_rv);
            return;
        }
    }

    json out = file_compare_output{comp_rv, errno};
    fmt::print("{}\n", out.dump(2));
}

void
file_compare_init(CLI::App& app) {

    // Create the option and subcommand objects
    auto opts = std::make_shared<file_compare_options>();
    auto* cmd = app.add_subcommand(
            "file_compare",
            "Execute the truncate() system call");

    // Add options to cmd, binding them to opts
    cmd->add_flag(
            "-v,--verbose",
            opts->verbose,
            "Produce human writeable output"
    );

    cmd->add_option(
                    "path_1",
                    opts->path_1,
                    "Path to first file"
            )
            ->required()
            ->type_name("");

    cmd->add_option(
                    "path_2",
                    opts->path_2,
                    "Path to second file"
            )
            ->required()
            ->type_name("");

    cmd->add_option(
                    "count",
                    opts->count,
                    "How many bytes to compare of each file"
            )
            ->required()
            ->type_name("");

    cmd->callback([opts]() {
        file_compare_exec(*opts);
    });
}