/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef IFS_LOG_UITIL_HPP
#define IFS_LOG_UITIL_HPP

#include <spdlog/spdlog.h>


spdlog::level::level_enum get_spdlog_level(std::string level_str);
spdlog::level::level_enum get_spdlog_level(unsigned long level);
void setup_loggers(const std::vector<std::string>& loggers,
        spdlog::level::level_enum level, const std::string& path);


#endif
