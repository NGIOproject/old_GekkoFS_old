/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <global/configure.hpp>
#include <client/rpc/ld_rpc_metadentry.hpp>
#include "client/preload.hpp"
#include "client/logging.hpp"
#include "client/preload_util.hpp"
#include "client/open_dir.hpp"
#include <global/rpc/rpc_utils.hpp>
#include <global/rpc/distributor.hpp>
#include <global/rpc/rpc_types.hpp>
#include <client/rpc/hg_rpcs.hpp>

namespace rpc_send  {

using namespace std;

int mk_node(const std::string& path, const mode_t mode) {

    int err = EUNKNOWN;
    auto endp = CTX->hosts().at(
            CTX->distributor()->locate_file_metadata(path));
    
    try {
        LOG(DEBUG, "Sending RPC ...");
        // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we can
        // retry for RPC_TRIES (see old commits with margo)
        // TODO(amiranda): hermes will eventually provide a post(endpoint) 
        // returning one result and a broadcast(endpoint_set) returning a 
        // result_set. When that happens we can remove the .at(0) :/
        auto out = 
            ld_network_service->post<gkfs::rpc::create>(endp, path, mode).get().at(0);
        err = out.err();
        LOG(DEBUG, "Got response success: {}", err);

        if(out.err()) {
            errno = out.err();
            return -1;
        }

    } catch(const std::exception& ex) {
        LOG(ERROR, "while getting rpc output");
        errno = EBUSY;
        return -1;
    }

    return err;
}

int stat(const std::string& path, string& attr) {

    auto endp = CTX->hosts().at(
            CTX->distributor()->locate_file_metadata(path));
    
    try {
        LOG(DEBUG, "Sending RPC ...");
        // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we can
        // retry for RPC_TRIES (see old commits with margo)
        // TODO(amiranda): hermes will eventually provide a post(endpoint) 
        // returning one result and a broadcast(endpoint_set) returning a 
        // result_set. When that happens we can remove the .at(0) :/
        auto out = 
            ld_network_service->post<gkfs::rpc::stat>(endp, path).get().at(0);
        LOG(DEBUG, "Got response success: {}", out.err());

        if(out.err() != 0) {
            errno = out.err();
            return -1;
        }

        attr = out.db_val();
        return 0;

    } catch(const std::exception& ex) {
        LOG(ERROR, "while getting rpc output");
        errno = EBUSY;
        return -1;
    }

    return 0;
}

int decr_size(const std::string& path, size_t length) {

    auto endp = CTX->hosts().at(
        CTX->distributor()->locate_file_metadata(path));

    try {

        LOG(DEBUG, "Sending RPC ...");
        // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we can
        // retry for RPC_TRIES (see old commits with margo)
        // TODO(amiranda): hermes will eventually provide a post(endpoint) 
        // returning one result and a broadcast(endpoint_set) returning a 
        // result_set. When that happens we can remove the .at(0) :/
        auto out = 
            ld_network_service->post<gkfs::rpc::decr_size>(
                    endp, path, length).get().at(0);

        LOG(DEBUG, "Got response success: {}", out.err());

        if(out.err() != 0) {
            errno = out.err();
            return -1;
        }

        return 0;

    } catch(const std::exception& ex) {
        LOG(ERROR, "while getting rpc output");
        errno = EBUSY;
        return -1;
    }
}

int rm_node(const std::string& path, const bool remove_metadentry_only, const ssize_t size) {

    // if only the metadentry should be removed, send one rpc to the
    // metadentry's responsible node to remove the metadata
    // else, send an rpc to all hosts and thus broadcast chunk_removal.
    if(remove_metadentry_only) {

        auto endp = CTX->hosts().at(
            CTX->distributor()->locate_file_metadata(path));

        try {

            LOG(DEBUG, "Sending RPC ...");
            // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we can
            // retry for RPC_TRIES (see old commits with margo)
            // TODO(amiranda): hermes will eventually provide a post(endpoint) 
            // returning one result and a broadcast(endpoint_set) returning a 
            // result_set. When that happens we can remove the .at(0) :/
            auto out = 
                ld_network_service->post<gkfs::rpc::remove>(endp, path).get().at(0);

            LOG(DEBUG, "Got response success: {}", out.err());

            if(out.err() != 0) {
                errno = out.err();
                return -1;
            }
            
            return 0;

        } catch(const std::exception& ex) {
            LOG(ERROR, "while getting rpc output");
            errno = EBUSY;
            return -1;
        }

        return 0;
    }

    std::vector<hermes::rpc_handle<gkfs::rpc::remove>> handles;

	// Small files
    if(static_cast<std::size_t>(size / CHUNKSIZE) < CTX->hosts().size()) {

        auto endp = CTX->hosts().at(
                CTX->distributor()->locate_file_metadata(path));

        try {
            LOG(DEBUG, "Sending RPC to host: {}", endp.to_string());
            gkfs::rpc::remove::input in(path);
            handles.emplace_back(
                    ld_network_service->post<gkfs::rpc::remove>(endp,in));

            uint64_t chnk_start = 0;
            uint64_t chnk_end = size/CHUNKSIZE;

            for (uint64_t chnk_id = chnk_start; chnk_id <= chnk_end; chnk_id++) {
                const auto target = CTX->hosts().at(
                        CTX->distributor()->locate_data(path, chnk_id));

                LOG(DEBUG, "Sending RPC to host: {}", target.to_string());

                handles.emplace_back(
                        ld_network_service->post<gkfs::rpc::remove>(target, in));
            }
        } catch (const std::exception & ex) {
            LOG(ERROR, "Failed to send reduced remove requests");
            throw std::runtime_error(
                    "Failed to forward non-blocking rpc request");
        }
    }
    else {	// "Big" files 
        for (const auto& endp : CTX->hosts()) {
            try {
                LOG(DEBUG, "Sending RPC to host: {}", endp.to_string());

                gkfs::rpc::remove::input in(path);

                // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that
                // we can retry for RPC_TRIES (see old commits with margo)
                // TODO(amiranda): hermes will eventually provide a post(endpoint) 
                // returning one result and a broadcast(endpoint_set) returning a 
                // result_set. When that happens we can remove the .at(0) :/
                //
                //

                handles.emplace_back(
                        ld_network_service->post<gkfs::rpc::remove>(endp, in));

            } catch (const std::exception& ex) {
                // TODO(amiranda): we should cancel all previously posted requests 
                // here, unfortunately, Hermes does not support it yet :/
                LOG(ERROR, "Failed to send request to host: {}", 
                        endp.to_string());
                throw std::runtime_error(
                        "Failed to forward non-blocking rpc request");
            }
        }
    }
    // wait for RPC responses
    bool got_error = false;

    for(const auto& h : handles) {

        try {
            // XXX We might need a timeout here to not wait forever for an
            // output that never comes?
            auto out = h.get().at(0);

            if(out.err() != 0) {
                LOG(ERROR, "received error response: {}", out.err());
                got_error = true;
                errno = out.err();
            }
        } catch(const std::exception& ex) {
            LOG(ERROR, "while getting rpc output");
            got_error = true;
            errno = EBUSY;
        }
    }

    return got_error ? -1 : 0;

}


int update_metadentry(const string& path, const Metadata& md, const MetadentryUpdateFlags& md_flags) {

    auto endp = CTX->hosts().at(
        CTX->distributor()->locate_file_metadata(path));

    try {

        LOG(DEBUG, "Sending RPC ...");
        // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we can
        // retry for RPC_TRIES (see old commits with margo)
        // TODO(amiranda): hermes will eventually provide a post(endpoint) 
        // returning one result and a broadcast(endpoint_set) returning a 
        // result_set. When that happens we can remove the .at(0) :/
        auto out = 
            ld_network_service->post<gkfs::rpc::update_metadentry>(
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

        if(out.err() != 0) {
            errno = out.err();
            return -1;
        }

        return 0;

    } catch(const std::exception& ex) {
        LOG(ERROR, "while getting rpc output");
        errno = EBUSY;
        return -1;
    }
}

int update_metadentry_size(const string& path, const size_t size, const off64_t offset, const bool append_flag,
                                    off64_t& ret_size) {

    auto endp = CTX->hosts().at(
        CTX->distributor()->locate_file_metadata(path));

    try {

        LOG(DEBUG, "Sending RPC ...");
        // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we can
        // retry for RPC_TRIES (see old commits with margo)
        // TODO(amiranda): hermes will eventually provide a post(endpoint) 
        // returning one result and a broadcast(endpoint_set) returning a 
        // result_set. When that happens we can remove the .at(0) :/
        auto out = 
            ld_network_service->post<gkfs::rpc::update_metadentry_size>(
                    endp, path, size, offset,
                    bool_to_merc_bool(append_flag)).get().at(0);

        LOG(DEBUG, "Got response success: {}", out.err());

        if(out.err() != 0) {
            errno = out.err();
            return -1;
        }

        ret_size = out.ret_size();
        return out.err();

        return 0;

    } catch(const std::exception& ex) {
        LOG(ERROR, "while getting rpc output");
        errno = EBUSY;
        ret_size = 0;
        return EUNKNOWN;
    }
}

int get_metadentry_size(const std::string& path, off64_t& ret_size) {

    auto endp = CTX->hosts().at(
        CTX->distributor()->locate_file_metadata(path));

    try {

        LOG(DEBUG, "Sending RPC ...");
        // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we can
        // retry for RPC_TRIES (see old commits with margo)
        // TODO(amiranda): hermes will eventually provide a post(endpoint) 
        // returning one result and a broadcast(endpoint_set) returning a 
        // result_set. When that happens we can remove the .at(0) :/
        auto out = 
            ld_network_service->post<gkfs::rpc::get_metadentry_size>(
                    endp, path).get().at(0);

        LOG(DEBUG, "Got response success: {}", out.err());

        ret_size = out.ret_size();
        return out.err();

    } catch(const std::exception& ex) {
        LOG(ERROR, "while getting rpc output");
        errno = EBUSY;
        ret_size = 0;
        return EUNKNOWN;
    }
}

/**
 * Sends an RPC request to a specific node to push all chunks that belong to him
 */
void get_dirents(OpenDir& open_dir){

    auto const root_dir = open_dir.path();
    auto const targets = 
        CTX->distributor()->locate_directory_metadata(root_dir);

    /* preallocate receiving buffer. The actual size is not known yet.
     *
     * On C++14 make_unique function also zeroes the newly allocated buffer.
     * It turns out that this operation is increadibly slow for such a big
     * buffer. Moreover we don't need a zeroed buffer here.
     */
    auto large_buffer = 
        std::unique_ptr<char[]>(new char[RPC_DIRENTS_BUFF_SIZE]);

    //XXX there is a rounding error here depending on the number of targets...
    const std::size_t per_host_buff_size = 
        RPC_DIRENTS_BUFF_SIZE / targets.size();

    // expose local buffers for RMA from servers
    std::vector<hermes::exposed_memory> exposed_buffers;
    exposed_buffers.reserve(targets.size());

    for(std::size_t i = 0; i < targets.size(); ++i) {
        try {
            exposed_buffers.emplace_back(
                ld_network_service->expose(
                    std::vector<hermes::mutable_buffer>{
                        hermes::mutable_buffer{
                            large_buffer.get() + (i * per_host_buff_size),
                            per_host_buff_size
                        }
                    },
                    hermes::access_mode::write_only));
        } catch (const std::exception& ex) {
            throw std::runtime_error("Failed to expose buffers for RMA");
        }
    }

    // send RPCs
    std::vector<hermes::rpc_handle<gkfs::rpc::get_dirents>> handles;

    for(std::size_t i = 0; i < targets.size(); ++i) {

        LOG(DEBUG, "target_host: {}", targets[i]);

        // Setup rpc input parameters for each host
        auto endp = CTX->hosts().at(targets[i]);

        gkfs::rpc::get_dirents::input in(root_dir, exposed_buffers[i]);

        try {

            LOG(DEBUG, "Sending RPC to host: {}", targets[i]);
            handles.emplace_back(
                ld_network_service->post<gkfs::rpc::get_dirents>(endp, in));
        } catch(const std::exception& ex) {
            LOG(ERROR, "Unable to send non-blocking get_dirents() "
                "on {} [peer: {}]", root_dir, targets[i]);
            throw std::runtime_error("Failed to post non-blocking RPC request");
        }
    }

    // wait for RPC responses
    for(std::size_t i = 0; i < handles.size(); ++i) {

        gkfs::rpc::get_dirents::output out;

        try {
            // XXX We might need a timeout here to not wait forever for an
            // output that never comes?
            out = handles[i].get().at(0);

            if(out.err() != 0) {
                throw std::runtime_error(
                        fmt::format("Failed to retrieve dir entries from "
                                    "host '{}'. Error '{}', path '{}'", 
                                    targets[i], strerror(out.err()), root_dir));
            }
        } catch(const std::exception& ex) {
            throw std::runtime_error(
                    fmt::format("Failed to get rpc output.. [path: {}, "
                                "target host: {}]", root_dir, targets[i]));
        }

        // each server wrote information to its pre-defined region in 
        // large_buffer, recover it by computing the base_address for each
        // particular server and adding the appropriate offsets
        assert(exposed_buffers[i].count() == 1);
        void* base_ptr = exposed_buffers[i].begin()->data();

        bool* bool_ptr = reinterpret_cast<bool*>(base_ptr);
        char* names_ptr = reinterpret_cast<char*>(base_ptr) + 
                            (out.dirents_size() * sizeof(bool));

        for(std::size_t j = 0; j < out.dirents_size(); j++) {

            FileType ftype = (*bool_ptr) ? 
                                FileType::directory : 
                                FileType::regular;
            bool_ptr++;

            // Check that we are not outside the recv_buff for this specific host
            assert((names_ptr - reinterpret_cast<char*>(base_ptr)) > 0);
            assert(
                static_cast<unsigned long int>(
                    names_ptr - reinterpret_cast<char*>(base_ptr)) < 
                per_host_buff_size);

            auto name = std::string(names_ptr);
            names_ptr += name.size() + 1;

            open_dir.add(name, ftype);
        }
    }
}

#ifdef HAS_SYMLINKS

int mk_symlink(const std::string& path, const std::string& target_path) {

    auto endp = CTX->hosts().at(
        CTX->distributor()->locate_file_metadata(path));

    try {

        LOG(DEBUG, "Sending RPC ...");
        // TODO(amiranda): add a post() with RPC_TIMEOUT to hermes so that we can
        // retry for RPC_TRIES (see old commits with margo)
        // TODO(amiranda): hermes will eventually provide a post(endpoint) 
        // returning one result and a broadcast(endpoint_set) returning a 
        // result_set. When that happens we can remove the .at(0) :/
        auto out = 
            ld_network_service->post<gkfs::rpc::mk_symlink>(
                    endp, path, target_path).get().at(0);

        LOG(DEBUG, "Got response success: {}", out.err());

        if(out.err() != 0) {
            errno = out.err();
            return -1;
        }

        return 0;

    } catch(const std::exception& ex) {
        LOG(ERROR, "while getting rpc output");
        errno = EBUSY;
        return -1;
    }
}

#endif

} //end namespace rpc_send
