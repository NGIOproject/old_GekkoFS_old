/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef IFS_PATH_UTIL_HPP
#define IFS_PATH_UTIL_HPP

#include <string>
#include <vector>

constexpr unsigned int PATH_MAX_LEN = 4096; // 4k chars

constexpr char PSP = '/'; // PATH SEPARATOR


bool is_relative_path(const std::string& path);
bool is_absolute_path(const std::string& path);
bool has_trailing_slash(const std::string& path);

std::string prepend_path(const std::string& path, const char * raw_path);
std::string dirname(const std::string& path);
std::vector<std::string> split_path(const std::string& path);

#endif //IFS_PATH_UTIL_HPP
