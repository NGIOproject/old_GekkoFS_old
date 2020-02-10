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
#include <client/logging.hpp>
#include <client/preload_util.hpp>
#include <boost/type_traits/is_pointer.hpp> // see https://github.com/boostorg/tokenizer/issues/9
#include <boost/token_functions.hpp>
#include <boost/tokenizer.hpp>
#include <hermes.hpp>
#include <client/rpc/hg_rpcs.hpp>


namespace rpc_send {


/**
 * Gets fs configuration information from the running daemon and transfers it to the memory of the library
 * @return
 */
bool get_fs_config() {

    auto endp = CTX->hosts().at(CTX->local_host_id());
    gkfs::rpc::fs_config::output out;

    try {
        LOG(DEBUG, "Retrieving file system configurations from daemon");
        // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we can retry
        // for RPC_TRIES (see old commits with margo)
        // TODO(amiranda): hermes will eventually provide a post(endpoint) 
        // returning one result and a broadcast(endpoint_set) returning a 
        // result_set. When that happens we can remove the .at(0) :/
        out = ld_network_service->post<gkfs::rpc::fs_config>(endp).get().at(0);
    } catch (const std::exception& ex) {
        LOG(ERROR, "Retrieving fs configurations from daemon");
        return false;
    }

    CTX->mountdir(out.mountdir());
    LOG(INFO, "Mountdir: '{}'", CTX->mountdir());

    CTX->fs_conf()->rootdir = out.rootdir();
    CTX->fs_conf()->atime_state = out.atime_state();
    CTX->fs_conf()->mtime_state = out.mtime_state();
    CTX->fs_conf()->ctime_state = out.ctime_state();
    CTX->fs_conf()->link_cnt_state = out.link_cnt_state();
    CTX->fs_conf()->blocks_state = out.blocks_state();
    CTX->fs_conf()->uid = out.uid();
    CTX->fs_conf()->gid = out.gid();

    LOG(DEBUG, "Got response with mountdir {}", out.mountdir());

    return true;
}


}
