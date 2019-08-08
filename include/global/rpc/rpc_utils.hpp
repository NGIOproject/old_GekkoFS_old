/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#ifndef IFS_RPC_UTILS_HPP
#define IFS_RPC_UTILS_HPP

extern "C" {
#include <mercury_types.h>
#include <mercury_proc_string.h>
#include <margo.h>
}
#include <string>

template<typename I, typename O>
inline hg_return_t rpc_cleanup(hg_handle_t* handle, I* input, O* output, hg_bulk_t* bulk_handle) {
    auto ret = HG_SUCCESS;
    if (bulk_handle) {
        ret = margo_bulk_free(*bulk_handle);
        if (ret != HG_SUCCESS)
            return ret;
    }
    if (input && handle) {
        ret = margo_free_input(*handle, input);
        if (ret != HG_SUCCESS)
            return ret;
    }
    if (output && handle) {
        ret = margo_free_output(*handle, output);
        if (ret != HG_SUCCESS)
            return ret;
    }
    if (handle) {
        ret = margo_destroy(*handle);
        if (ret != HG_SUCCESS)
            return ret;
    }
    return ret;
}

template<typename I, typename O>
inline hg_return_t rpc_cleanup_respond(hg_handle_t* handle, I* input, O* output, hg_bulk_t* bulk_handle) {
    auto ret = HG_SUCCESS;
    if (output && handle) {
        ret = margo_respond(*handle, output);
        if (ret != HG_SUCCESS)
            return ret;
    }
    return rpc_cleanup(handle, input, static_cast<O*>(nullptr), bulk_handle);

}

hg_bool_t bool_to_merc_bool(bool state);

std::string get_my_hostname(bool short_hostname = false);

std::string get_host_by_name(const std::string & hostname);

bool is_handle_sm(margo_instance_id mid, const hg_addr_t& addr);

#endif //IFS_RPC_UTILS_HPP
