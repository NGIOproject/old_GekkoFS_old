/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <hermes.hpp>
#include <client/rpc/hg_rpcs.hpp>

namespace hermes { namespace detail {

//==============================================================================
// register request types so that they can be used by users and the engine
//
void
register_user_request_types() {
    (void) registered_requests().add<gkfs::rpc::fs_config>();
    (void) registered_requests().add<gkfs::rpc::create>();
    (void) registered_requests().add<gkfs::rpc::stat>();
    (void) registered_requests().add<gkfs::rpc::remove>();
    (void) registered_requests().add<gkfs::rpc::decr_size>();
    (void) registered_requests().add<gkfs::rpc::update_metadentry>();
    (void) registered_requests().add<gkfs::rpc::get_metadentry_size>();
    (void) registered_requests().add<gkfs::rpc::update_metadentry_size>();

#ifdef HAS_SYMLINKS
    (void) registered_requests().add<gkfs::rpc::mk_symlink>();
#endif // HAS_SYMLINKS

    (void) registered_requests().add<gkfs::rpc::write_data>();
    (void) registered_requests().add<gkfs::rpc::read_data>();
    (void) registered_requests().add<gkfs::rpc::trunc_data>();
    (void) registered_requests().add<gkfs::rpc::get_dirents>();
    (void) registered_requests().add<gkfs::rpc::chunk_stat>();

}

}} // namespace hermes::detail
