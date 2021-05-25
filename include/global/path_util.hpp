/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GEKKOFS_PATH_UTIL_HPP
#define GEKKOFS_PATH_UTIL_HPP

#include <string>
#include <vector>

namespace gkfs {
namespace path {

constexpr unsigned int max_length = 4096; // 4k chars

constexpr char separator = '/'; // PATH SEPARATOR

bool is_relative(const std::string& path);

bool is_absolute(const std::string& path);

bool has_trailing_slash(const std::string& path);

std::string prepend_path(const std::string& path, const char* raw_path);

std::string absolute_to_relative(const std::string& root_path, const std::string& absolute_path); // unused ATM

std::string dirname(const std::string& path);

std::vector<std::string> split_path(const std::string& path);

} // namespace gkfs
} // namespace path

#endif //GEKKOFS_PATH_UTIL_HPP
