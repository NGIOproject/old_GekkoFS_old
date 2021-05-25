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
#include <reflection.hpp>
#include <serialize.hpp>

/* C includes */
#include <sys/types.h>
#include <unistd.h>

using json = nlohmann::json;

struct truncate_options {
    bool verbose{};
    std::string path{};
    ::off_t length{};

    REFL_DECL_STRUCT(truncate_options,
        REFL_DECL_MEMBER(bool, verbose),
        REFL_DECL_MEMBER(std::string, path),
        REFL_DECL_MEMBER(::off_t, length)
    );
};

struct truncate_output {
    int retval;
    int errnum;

    REFL_DECL_STRUCT(truncate_output,
        REFL_DECL_MEMBER(int, retval),
        REFL_DECL_MEMBER(int, errnum)
    );
};

void
to_json(json& record, 
        const truncate_output& out) {
    record = serialize(out);
}

void 
truncate_exec(const truncate_options& opts) {

    auto rv = ::truncate(opts.path.c_str(), opts.length);
    if (rv == -1) {
        if (opts.verbose) {
            fmt::print("truncate(path=\"{}\", length={}) = {}, errno: {} [{}]\n",
                       opts.path, opts.length, rv, errno, ::strerror(errno));
            return;
        }
    }

    json out = truncate_output{rv, errno};
    fmt::print("{}\n", out.dump(2));
}

void
truncate_init(CLI::App& app) {

    // Create the option and subcommand objects
    auto opts = std::make_shared<truncate_options>();
    auto* cmd = app.add_subcommand(
            "truncate",
            "Execute the truncate() system call");

    // Add options to cmd, binding them to opts
    cmd->add_flag(
            "-v,--verbose",
            opts->verbose,
            "Produce human writeable output"
        );

    cmd->add_option(
            "path",
            opts->path,
            "Path to file"
        )
        ->required()
        ->type_name("");

    cmd->add_option(
            "length",
            opts->length,
            "Truncate to a size precisely length bytes"
        )
        ->required()
        ->type_name("");

    cmd->callback([opts]() { 
        truncate_exec(*opts);
    });
}


