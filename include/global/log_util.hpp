/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GEKKOFS_LOG_UITIL_HPP
#define GEKKOFS_LOG_UITIL_HPP

#include <spdlog/spdlog.h>

namespace gkfs {
namespace log {

spdlog::level::level_enum get_level(std::string level_str);

spdlog::level::level_enum get_level(unsigned long level);

void setup(const std::vector<std::string>& loggers, spdlog::level::level_enum level, const std::string& path);
}
}

#endif
