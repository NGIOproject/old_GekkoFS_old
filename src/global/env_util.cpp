/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <global/env_util.hpp>

#include <string>
#include <cstdlib>

using namespace std;

string gkfs::env::get_var(const string& name, const string& default_value) {
    const char* const val = ::secure_getenv(name.c_str());
    return val != nullptr ? string(val) : default_value;
}
