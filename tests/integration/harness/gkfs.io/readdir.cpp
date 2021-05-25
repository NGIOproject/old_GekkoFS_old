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
#include <dirent.h>
#include <unistd.h>

using json = nlohmann::json;

struct readdir_options {
    bool verbose;
    std::string pathname;
    ::size_t count;

    REFL_DECL_STRUCT(readdir_options,
        REFL_DECL_MEMBER(bool, verbose),
        REFL_DECL_MEMBER(std::string, pathname),
        REFL_DECL_MEMBER(::size_t, count)
    );
};

struct readdir_output {
    std::vector<struct ::dirent> dirents;
    int errnum;

    REFL_DECL_STRUCT(readdir_output,
        REFL_DECL_MEMBER(std::vector<struct ::dirent>, dirents),
        REFL_DECL_MEMBER(int, errnum)
    );
};

void
to_json(json& record, 
        const readdir_output& out) {
    record = serialize(out);
}

void 
readdir_exec(const readdir_options& opts) {

    ::DIR* dirp = ::opendir(opts.pathname.c_str());

    if(dirp == NULL) {
        if(opts.verbose) {
            fmt::print("readdir(pathname=\"{}\") = {}, errno: {} [{}]\n", 
                    opts.pathname, "NULL", errno, ::strerror(errno));
            return;
        }

        json out = readdir_output{{}, errno};
        fmt::print("{}\n", out.dump(2));

        return;
    }

    io::buffer buf(opts.count);

    std::vector<struct ::dirent> entries;
    struct ::dirent* entry;

    while((entry = ::readdir(dirp)) != NULL) {
        entries.push_back(*entry);
    }

    if(opts.verbose) {
        fmt::print("readdir(pathname=\"{}\") = [\n{}],\nerrno: {} [{}]\n", 
                   opts.pathname, fmt::join(entries, ",\n"), errno, ::strerror(errno));
        return;
    }

    json out = readdir_output{entries, errno};
    fmt::print("{}\n", out.dump(2));
}

void
readdir_init(CLI::App& app) {

    // Create the option and subcommand objects
    auto opts = std::make_shared<readdir_options>();
    auto* cmd = app.add_subcommand(
            "readdir", 
            "Execute the readdir() system call");

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

    cmd->callback([opts]() { 
        readdir_exec(*opts); 
    });
}


