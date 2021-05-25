/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <string>
#include <vector>

namespace gkfs {
namespace path {

unsigned int match_components(const std::string& path, unsigned int& path_components,
                              const std::vector<std::string>& components);

bool resolve(const std::string& path, std::string& resolved, bool resolve_last_link = true);

std::string get_sys_cwd();

void set_sys_cwd(const std::string& path);

void set_env_cwd(const std::string& path);

void unset_env_cwd();

void init_cwd();

void set_cwd(const std::string& path, bool internal);

} // namespace path
} // namespace gkfs
