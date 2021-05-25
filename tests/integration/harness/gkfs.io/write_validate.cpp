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

struct write_validate_options {
    bool verbose;
    std::string pathname;
    ::size_t count;

    REFL_DECL_STRUCT(write_validate_options,
        REFL_DECL_MEMBER(bool, verbose),
        REFL_DECL_MEMBER(std::string, pathname),
        REFL_DECL_MEMBER(::size_t, count)
    );
};

struct write_validate_output {
    int retval;
    int errnum;

    REFL_DECL_STRUCT(write_validate_output,
        REFL_DECL_MEMBER(int, retval),
        REFL_DECL_MEMBER(int, errnum)
    );
};

void
to_json(json& record, 
        const write_validate_output& out) {
    record = serialize(out);
}

void 
write_validate_exec(const write_validate_options& opts) {

    int fd = ::open(opts.pathname.c_str(), O_WRONLY);

    if(fd == -1) {
        if(opts.verbose) {
            fmt::print("write_validate(pathname=\"{}\", count={}) = {}, errno: {} [{}]\n", 
                    opts.pathname, opts.count, fd, errno, ::strerror(errno));
            return;
        }

        json out = write_validate_output{fd, errno};
        fmt::print("{}\n", out.dump(2));

        return;
    }


    std::string data = "";
    for (::size_t i = 0 ; i < opts.count; i++)
    {
        data += char((i%10)+'0');
    }

    io::buffer buf(data);

    auto rv = ::write(fd, buf.data(), opts.count);

    if(opts.verbose) {
        fmt::print("write_validate(pathname=\"{}\", count={}) = {}, errno: {} [{}]\n", 
                   opts.pathname, opts.count, rv, errno, ::strerror(errno));
        return;
    }

    if (rv < 0 or ::size_t(rv) != opts.count) {
        json out = write_validate_output{(int)rv, errno};
        fmt::print("{}\n", out.dump(2));
        return;
    }


    io::buffer bufread(opts.count);

    size_t total = 0;
    do{
        rv = ::read(fd, bufread.data(), opts.count-total);
        total += rv;
    } while (rv > 0 and total < opts.count);

    if (rv < 0 and total != opts.count) {
        json out = write_validate_output{(int)rv, errno};
        fmt::print("{}\n", out.dump(2));
        return;
    }

    if ( memcmp(buf.data(),bufread.data(),opts.count) ) {
        rv = 1;
        errno = 0;
        json out = write_validate_output{(int)rv, errno};
        fmt::print("{}\n", out.dump(2));
        return;
    }
    else  {
        rv = 2;
        errno = EINVAL;
        json out = write_validate_output{(int)-1, errno};
        fmt::print("{}\n", out.dump(2));
    }

}

void
write_validate_init(CLI::App& app) {

    // Create the option and subcommand objects
    auto opts = std::make_shared<write_validate_options>();
    auto* cmd = app.add_subcommand(
            "write_validate", 
            "Execute the write()-read() system call and compare the content of the buffer");

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
            "count", 
            opts->count,
            "Number of bytes to test"
        )
        ->required()
        ->type_name("");

    cmd->callback([opts]() { 
        write_validate_exec(*opts); 
    });
}


