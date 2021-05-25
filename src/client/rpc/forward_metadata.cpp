/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <client/rpc/forward_metadata.hpp>
#include <client/preload.hpp>
#include <client/logging.hpp>
#include <client/preload_util.hpp>
#include <client/open_dir.hpp>
#include <client/rpc/rpc_types.hpp>

#include <global/rpc/rpc_util.hpp>
#include <global/rpc/distributor.hpp>
#include <global/rpc/rpc_types.hpp>

using namespace std;

namespace gkfs {
namespace rpc {

/*
 * This file includes all metadata RPC calls.
 * NOTE: No errno is defined here!
 */

/**
 * Send an RPC for a create request
 * @param path
 * @param mode
 * @return error code
 */
int forward_create(const std::string& path, const mode_t mode) {

    auto endp = CTX->hosts().at(CTX->distributor()->locate_file_metadata(path));

    try {
        LOG(DEBUG, "Sending RPC ...");
        // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we can
        // retry for RPC_TRIES (see old commits with margo)
        // TODO(amiranda): hermes will eventually provide a post(endpoint)
        // returning one result and a broadcast(endpoint_set) returning a
        // result_set. When that happens we can remove the .at(0) :/
        auto out = ld_network_service->post<gkfs::rpc::create>(endp, path, mode).get().at(0);
        LOG(DEBUG, "Got response success: {}", out.err());

        return out.err() ? out.err() : 0;
    } catch (const std::exception& ex) {
        LOG(ERROR, "while getting rpc output");
        return EBUSY;
    }
}

/**
 * Send an RPC for a stat request
 * @param path
 * @param attr
 * @return error code
 */
int forward_stat(const std::string& path, string& attr) {

    auto endp = CTX->hosts().at(CTX->distributor()->locate_file_metadata(path));

    try {
        LOG(DEBUG, "Sending RPC ...");
        // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we can
        // retry for RPC_TRIES (see old commits with margo)
        // TODO(amiranda): hermes will eventually provide a post(endpoint)
        // returning one result and a broadcast(endpoint_set) returning a
        // result_set. When that happens we can remove the .at(0) :/
        auto out = ld_network_service->post<gkfs::rpc::stat>(endp, path).get().at(0);
        LOG(DEBUG, "Got response success: {}", out.err());

        if (out.err())
            return out.err();

        attr = out.db_val();
    } catch (const std::exception& ex) {
        LOG(ERROR, "while getting rpc output");
        return EBUSY;
    }
    return 0;
}

/**
 * Send an RPC for a remove request. This removes metadata and all data chunks possible distributed across many daemons.
 * Optimizations are in place for small files (file_size / chunk_size) < number_of_daemons where no broadcast to all
 * daemons is used to remove all chunks. Otherwise, a broadcast to all daemons is used.
 * @param path
 * @param remove_metadentry_only
 * @param size
 * @return error code
 */
int forward_remove(const std::string& path, const bool remove_metadentry_only, const ssize_t size) {

    // if only the metadentry should be removed, send one rpc to the
    // metadentry's responsible node to remove the metadata
    // else, send an rpc to all hosts and thus broadcast chunk_removal.
    if (remove_metadentry_only) {
        auto endp = CTX->hosts().at(CTX->distributor()->locate_file_metadata(path));

        try {

            LOG(DEBUG, "Sending RPC ...");
            // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we can
            // retry for RPC_TRIES (see old commits with margo)
            // TODO(amiranda): hermes will eventually provide a post(endpoint)
            // returning one result and a broadcast(endpoint_set) returning a
            // result_set. When that happens we can remove the .at(0) :/
            auto out = ld_network_service->post<gkfs::rpc::remove>(endp, path).get().at(0);

            LOG(DEBUG, "Got response success: {}", out.err());

            return out.err() ? out.err() : 0;
        } catch (const std::exception& ex) {
            LOG(ERROR, "while getting rpc output");
            return EBUSY;
        }
    }

    std::vector<hermes::rpc_handle<gkfs::rpc::remove>> handles;

    // Small files
    if (static_cast<std::size_t>(size / gkfs::config::rpc::chunksize) < CTX->hosts().size()) {
        const auto metadata_host_id = CTX->distributor()->locate_file_metadata(path);
        const auto endp_metadata = CTX->hosts().at(metadata_host_id);

        try {
            LOG(DEBUG, "Sending RPC to host: {}", endp_metadata.to_string());
            gkfs::rpc::remove::input in(path);
            handles.emplace_back(ld_network_service->post<gkfs::rpc::remove>(endp_metadata, in));

            uint64_t chnk_start = 0;
            uint64_t chnk_end = size / gkfs::config::rpc::chunksize;

            for (uint64_t chnk_id = chnk_start; chnk_id <= chnk_end; chnk_id++) {
                const auto chnk_host_id = CTX->distributor()->locate_data(path, chnk_id);
                /*
                 * If the chnk host matches the metadata host the remove request as already been sent
                 * as part of the metadata remove request.
                 */
                if (chnk_host_id == metadata_host_id)
                    continue;
                const auto endp_chnk = CTX->hosts().at(chnk_host_id);

                LOG(DEBUG, "Sending RPC to host: {}", endp_chnk.to_string());

                handles.emplace_back(ld_network_service->post<gkfs::rpc::remove>(endp_chnk, in));
            }
        } catch (const std::exception& ex) {
            LOG(ERROR, "Failed to forward non-blocking rpc request reduced remove requests");
            return EBUSY;
        }
    } else {    // "Big" files
        for (const auto& endp : CTX->hosts()) {
            try {
                LOG(DEBUG, "Sending RPC to host: {}", endp.to_string());

                gkfs::rpc::remove::input in(path);

                // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that
                // we can retry for RPC_TRIES (see old commits with margo)
                // TODO(amiranda): hermes will eventually provide a post(endpoint)
                // returning one result and a broadcast(endpoint_set) returning a
                // result_set. When that happens we can remove the .at(0) :/

                handles.emplace_back(ld_network_service->post<gkfs::rpc::remove>(endp, in));

            } catch (const std::exception& ex) {
                // TODO(amiranda): we should cancel all previously posted requests
                // here, unfortunately, Hermes does not support it yet :/
                LOG(ERROR, "Failed to forward non-blocking rpc request to host: {}",
                    endp.to_string());
                return EBUSY;
            }
        }
    }
    // wait for RPC responses
    auto err = 0;
    for (const auto& h : handles) {
        try {
            // XXX We might need a timeout here to not wait forever for an
            // output that never comes?
            auto out = h.get().at(0);

            if (out.err() != 0) {
                LOG(ERROR, "received error response: {}", out.err());
                err = out.err();
            }
        } catch (const std::exception& ex) {
            LOG(ERROR, "while getting rpc output");
            err = EBUSY;
        }
    }
    return err;
}

/**
 * Send an RPC for a decrement file size request. This is for example used during a truncate() call.
 * @param path
 * @param length
 * @return error code
 */
int forward_decr_size(const std::string& path, size_t length) {

    auto endp = CTX->hosts().at(CTX->distributor()->locate_file_metadata(path));

    try {
        LOG(DEBUG, "Sending RPC ...");
        // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we can
        // retry for RPC_TRIES (see old commits with margo)
        // TODO(amiranda): hermes will eventually provide a post(endpoint)
        // returning one result and a broadcast(endpoint_set) returning a
        // result_set. When that happens we can remove the .at(0) :/
        auto out = ld_network_service->post<gkfs::rpc::decr_size>(endp, path, length).get().at(0);

        LOG(DEBUG, "Got response success: {}", out.err());

        return out.err() ? out.err() : 0;
    } catch (const std::exception& ex) {
        LOG(ERROR, "while getting rpc output");
        return EBUSY;
    }
}


/**
 * Send an RPC for an update metadentry request.
 * NOTE: Currently unused.
 * @param path
 * @param md
 * @param md_flags
 * @return error code
 */
int forward_update_metadentry(const string& path, const gkfs::metadata::Metadata& md,
                              const gkfs::metadata::MetadentryUpdateFlags& md_flags) {

    auto endp = CTX->hosts().at(CTX->distributor()->locate_file_metadata(path));

    try {
        LOG(DEBUG, "Sending RPC ...");
        // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we can
        // retry for RPC_TRIES (see old commits with margo)
        // TODO(amiranda): hermes will eventually provide a post(endpoint)
        // returning one result and a broadcast(endpoint_set) returning a
        // result_set. When that happens we can remove the .at(0) :/
        auto out = ld_network_service->post<gkfs::rpc::update_metadentry>(
                endp,
                path,
                (md_flags.link_count ? md.link_count() : 0),
                /* mode */ 0,
                /* uid */  0,
                /* gid */  0,
                (md_flags.size ? md.size() : 0),
                (md_flags.blocks ? md.blocks() : 0),
                (md_flags.atime ? md.atime() : 0),
                (md_flags.mtime ? md.mtime() : 0),
                (md_flags.ctime ? md.ctime() : 0),
                bool_to_merc_bool(md_flags.link_count),
                /* mode_flag */ false,
                bool_to_merc_bool(md_flags.size),
                bool_to_merc_bool(md_flags.blocks),
                bool_to_merc_bool(md_flags.atime),
                bool_to_merc_bool(md_flags.mtime),
                bool_to_merc_bool(md_flags.ctime)).get().at(0);

        LOG(DEBUG, "Got response success: {}", out.err());

        return out.err() ? out.err() : 0;
    } catch (const std::exception& ex) {
        LOG(ERROR, "while getting rpc output");
        return EBUSY;
    }
}

/**
 * Send an RPC request for an update to the file size.
 * This is called during a write() call or similar
 * @param path
 * @param size
 * @param offset
 * @param append_flag
 * @return pair<error code, size after update>
 */
pair<int, off64_t>
forward_update_metadentry_size(const string& path, const size_t size, const off64_t offset, const bool append_flag) {

    auto endp = CTX->hosts().at(CTX->distributor()->locate_file_metadata(path));
    try {
        LOG(DEBUG, "Sending RPC ...");
        // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we can
        // retry for RPC_TRIES (see old commits with margo)
        // TODO(amiranda): hermes will eventually provide a post(endpoint)
        // returning one result and a broadcast(endpoint_set) returning a
        // result_set. When that happens we can remove the .at(0) :/
        auto out = ld_network_service->post<gkfs::rpc::update_metadentry_size>(
                endp, path, size, offset,
                bool_to_merc_bool(append_flag)).get().at(0);

        LOG(DEBUG, "Got response success: {}", out.err());

        if (out.err())
            return make_pair(out.err(), 0);
        else
            return make_pair(0, out.ret_size());
    } catch (const std::exception& ex) {
        LOG(ERROR, "while getting rpc output");
        return make_pair(EBUSY, 0);
    }
}

/**
 * Send an RPC request to get the current file size.
 * This is called during a lseek() call
 * @param path
 * @return pair<error code, file size>
 */
pair<int, off64_t> forward_get_metadentry_size(const std::string& path) {

    auto endp = CTX->hosts().at(CTX->distributor()->locate_file_metadata(path));

    try {
        LOG(DEBUG, "Sending RPC ...");
        // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we can
        // retry for RPC_TRIES (see old commits with margo)
        // TODO(amiranda): hermes will eventually provide a post(endpoint)
        // returning one result and a broadcast(endpoint_set) returning a
        // result_set. When that happens we can remove the .at(0) :/
        auto out = ld_network_service->post<gkfs::rpc::get_metadentry_size>(endp, path).get().at(0);

        LOG(DEBUG, "Got response success: {}", out.err());

        if (out.err())
            return make_pair(out.err(), 0);
        else
            return make_pair(0, out.ret_size());
    } catch (const std::exception& ex) {
        LOG(ERROR, "while getting rpc output");
        return make_pair(EBUSY, 0);
    }
}

/**
 * Send an RPC request to receive all entries of a directory.
 * @param open_dir
 * @return error code
 */
pair<int, shared_ptr<gkfs::filemap::OpenDir>> forward_get_dirents(const string& path) {

    LOG(DEBUG, "{}() enter for path '{}'", __func__, path)

    auto const targets = CTX->distributor()->locate_directory_metadata(path);

    /* preallocate receiving buffer. The actual size is not known yet.
     *
     * On C++14 make_unique function also zeroes the newly allocated buffer.
     * It turns out that this operation is increadibly slow for such a big
     * buffer. Moreover we don't need a zeroed buffer here.
     */
    auto large_buffer = std::unique_ptr<char[]>(new char[gkfs::config::rpc::dirents_buff_size]);

    //XXX there is a rounding error here depending on the number of targets...
    const std::size_t per_host_buff_size = gkfs::config::rpc::dirents_buff_size / targets.size();

    // expose local buffers for RMA from servers
    std::vector<hermes::exposed_memory> exposed_buffers;
    exposed_buffers.reserve(targets.size());

    for (std::size_t i = 0; i < targets.size(); ++i) {
        try {
            exposed_buffers.emplace_back(ld_network_service->expose(
                    std::vector<hermes::mutable_buffer>{
                            hermes::mutable_buffer{
                                    large_buffer.get() + (i * per_host_buff_size),
                                    per_host_buff_size
                            }
                    },
                    hermes::access_mode::write_only));
        } catch (const std::exception& ex) {
            LOG(ERROR, "{}() Failed to expose buffers for RMA. err '{}'", __func__, ex.what());
            return make_pair(EBUSY, nullptr);
        }
    }

    auto err = 0;
    // send RPCs
    std::vector<hermes::rpc_handle<gkfs::rpc::get_dirents>> handles;

    for (std::size_t i = 0; i < targets.size(); ++i) {

        // Setup rpc input parameters for each host
        auto endp = CTX->hosts().at(targets[i]);

        gkfs::rpc::get_dirents::input in(path, exposed_buffers[i]);

        try {
            LOG(DEBUG, "{}() Sending RPC to host: '{}'", __func__, targets[i]);
            handles.emplace_back(ld_network_service->post<gkfs::rpc::get_dirents>(endp, in));
        } catch (const std::exception& ex) {
            LOG(ERROR, "{}() Unable to send non-blocking get_dirents() on {} [peer: {}] err '{}'", __func__, path,
                targets[i], ex.what());
            err = EBUSY;
            break; // we need to gather responses from already sent RPCS
        }
    }

    LOG(INFO,
        "{}() path '{}' send rpc_srv_get_dirents() rpc to '{}' targets. per_host_buff_size '{}' Waiting on reply next and deserialize",
        __func__, path, targets.size(), per_host_buff_size);

    auto send_error = err != 0;
    auto open_dir = make_shared<gkfs::filemap::OpenDir>(path);
    // wait for RPC responses
    for (std::size_t i = 0; i < handles.size(); ++i) {

        gkfs::rpc::get_dirents::output out;

        try {
            // XXX We might need a timeout here to not wait forever for an
            // output that never comes?
            out = handles[i].get().at(0);
            // skip processing dirent data if there was an error during send
            // In this case all responses are gathered but their contents skipped
            if (send_error)
                continue;

            if (out.err() != 0) {
                LOG(ERROR, "{}() Failed to retrieve dir entries from host '{}'. Error '{}', path '{}'", __func__,
                    targets[i],
                    strerror(out.err()), path);
                err = out.err();
                // We need to gather all responses before exiting
                continue;
            }
        } catch (const std::exception& ex) {
            LOG(ERROR, "{}() Failed to get rpc output.. [path: {}, target host: {}] err '{}'", __func__, path,
                targets[i], ex.what());
            err = EBUSY;
            // We need to gather all responses before exiting
            continue;
        }

        // each server wrote information to its pre-defined region in
        // large_buffer, recover it by computing the base_address for each
        // particular server and adding the appropriate offsets
        assert(exposed_buffers[i].count() == 1);
        void* base_ptr = exposed_buffers[i].begin()->data();

        bool* bool_ptr = reinterpret_cast<bool*>(base_ptr);
        char* names_ptr = reinterpret_cast<char*>(base_ptr) + (out.dirents_size() * sizeof(bool));

        for (std::size_t j = 0; j < out.dirents_size(); j++) {

            gkfs::filemap::FileType ftype = (*bool_ptr) ? gkfs::filemap::FileType::directory
                                                        : gkfs::filemap::FileType::regular;
            bool_ptr++;

            // Check that we are not outside the recv_buff for this specific host
            assert((names_ptr - reinterpret_cast<char*>(base_ptr)) > 0);
            assert(static_cast<unsigned long int>(names_ptr - reinterpret_cast<char*>(base_ptr)) < per_host_buff_size);

            auto name = std::string(names_ptr);
            // number of characters in entry + \0 terminator
            names_ptr += name.size() + 1;

            open_dir->add(name, ftype);
        }
    }
    return make_pair(err, open_dir);
}

#ifdef HAS_SYMLINKS

/**
 * Send an RPC request to create a symlink.
 * @param path
 * @param target_path
 * @return error code
 */
int forward_mk_symlink(const std::string& path, const std::string& target_path) {

    auto endp = CTX->hosts().at(CTX->distributor()->locate_file_metadata(path));

    try {
        LOG(DEBUG, "Sending RPC ...");
        // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we can
        // retry for RPC_TRIES (see old commits with margo)
        // TODO(amiranda): hermes will eventually provide a post(endpoint)
        // returning one result and a broadcast(endpoint_set) returning a
        // result_set. When that happens we can remove the .at(0) :/
        auto out = ld_network_service->post<gkfs::rpc::mk_symlink>(endp, path, target_path).get().at(0);

        LOG(DEBUG, "Got response success: {}", out.err());

        return out.err() ? out.err() : 0;
    } catch (const std::exception& ex) {
        LOG(ERROR, "while getting rpc output");
        return EBUSY;
    }
}

#endif

} // namespace rpc
} // namespace gkfs