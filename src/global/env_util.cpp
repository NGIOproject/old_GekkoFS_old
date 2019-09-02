/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <global/env_util.hpp>
#include <global/configure.hpp>
#include <cstdlib>
#include <stdexcept>


namespace gkfs {

using namespace std;

string get_env(const string& env_name) {
    char* env_value = secure_getenv(env_name.c_str());
    if (env_value == nullptr) {
        throw runtime_error("Environment variable not set: " + env_name);
    }
    return env_value;
}

string get_env_own(const string& env_name) {
    string env_key = ENV_PREFIX + env_name;
    return get_env(env_key);
}

}
