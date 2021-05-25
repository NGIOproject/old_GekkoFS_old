/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#include <daemon/daemon.hpp>
#include <daemon/handler/rpc_defs.hpp>

#include <global/rpc/rpc_types.hpp>

using namespace std;

/*
 * This file contains all Margo RPC handlers that are concerning data operations
 */

namespace {

hg_return_t rpc_srv_get_fs_config(hg_handle_t handle) {
    rpc_config_out_t out{};

    GKFS_DATA->spdlogger()->debug("{}() Got config RPC", __func__);

    // get fs config
    out.mountdir = GKFS_DATA->mountdir().c_str();
    out.rootdir = GKFS_DATA->rootdir().c_str();
    out.atime_state = static_cast<hg_bool_t>(GKFS_DATA->atime_state());
    out.mtime_state = static_cast<hg_bool_t>(GKFS_DATA->mtime_state());
    out.ctime_state = static_cast<hg_bool_t>(GKFS_DATA->ctime_state());
    out.link_cnt_state = static_cast<hg_bool_t>(GKFS_DATA->link_cnt_state());
    out.blocks_state = static_cast<hg_bool_t>(GKFS_DATA->blocks_state());
    out.uid = getuid();
    out.gid = getgid();
    GKFS_DATA->spdlogger()->debug("{}() Sending output configs back to library", __func__);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        GKFS_DATA->spdlogger()->error("{}() Failed to respond to client to serve file system configurations",
                                      __func__);
    }

    // Destroy handle when finished
    margo_destroy(handle);
    return HG_SUCCESS;
}

}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_get_fs_config)

