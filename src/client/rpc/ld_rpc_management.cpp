/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include "client/rpc/ld_rpc_management.hpp"
#include "global/rpc/rpc_types.hpp"
#include <client/preload_util.hpp>
#include <boost/type_traits/is_pointer.hpp> // see https://github.com/boostorg/tokenizer/issues/9
#include <boost/token_functions.hpp>
#include <boost/tokenizer.hpp>


namespace rpc_send {


/**
 * Gets fs configuration information from the running daemon and transfers it to the memory of the library
 * @return
 */
bool get_fs_config() {
    hg_handle_t handle;
    rpc_config_out_t out{};
    // fill in
    auto ret = margo_create(ld_margo_rpc_id, CTX->hosts().at(CTX->local_host_id()), rpc_config_id, &handle);
    if (ret != HG_SUCCESS) {
        CTX->log()->error("{}() creating handle for failed", __func__);
        return false;
    }
    CTX->log()->debug("{}() Forwarding request", __func__);
    for (int i = 0; i < RPC_TRIES; ++i) {
        ret = margo_forward_timed(handle, nullptr, RPC_TIMEOUT);
        if (ret == HG_SUCCESS) {
            break;
        }
        CTX->log()->warn("{}() Failed to forward request. Error: {}. Attempt {}/{}", __func__, HG_Error_to_string(ret), i+1, RPC_TRIES);
    }
    if (ret != HG_SUCCESS) {
        CTX->log()->error("{}() Failed to forward request. Giving up after {} attempts", __func__, RPC_TRIES);
        margo_destroy(handle);
        return false;
    }

    /* decode response */
    CTX->log()->debug("{}() Waiting for response", __func__);
    ret = margo_get_output(handle, &out);
    if (ret != HG_SUCCESS) {
        CTX->log()->error("{}() Retrieving fs configurations from daemon", __func__);
        margo_destroy(handle);
        return false;
    }

    CTX->mountdir(out.mountdir);
    CTX->log()->info("Mountdir: '{}'", CTX->mountdir());

    CTX->fs_conf()->rootdir = out.rootdir;
    CTX->fs_conf()->atime_state = out.atime_state;
    CTX->fs_conf()->mtime_state = out.mtime_state;
    CTX->fs_conf()->ctime_state = out.ctime_state;
    CTX->fs_conf()->link_cnt_state = out.link_cnt_state;
    CTX->fs_conf()->blocks_state = out.blocks_state;
    CTX->fs_conf()->uid = out.uid;
    CTX->fs_conf()->gid = out.gid;

    CTX->log()->debug("{}() Got response with mountdir {}", __func__, out.mountdir);

    /* clean up resources consumed by this rpc */
    margo_free_output(handle, &out);
    margo_destroy(handle);
    return true;
}


}