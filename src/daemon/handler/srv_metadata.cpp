/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#include <daemon/handler/rpc_defs.hpp>
#include <daemon/handler/rpc_util.hpp>
#include <daemon/backend/metadata/db.hpp>
#include <daemon/ops/metadentry.hpp>

#include <global/rpc/rpc_types.hpp>
#include <daemon/backend/data/chunk_storage.hpp>

using namespace std;

/*
 * This file contains all Margo RPC handlers that are concerning metadata operations
 */

namespace {

hg_return_t rpc_srv_create(hg_handle_t handle) {
    rpc_mk_node_in_t in;
    rpc_err_out_t out;

    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        GKFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    GKFS_DATA->spdlogger()->debug("{}() Got RPC with path '{}'", __func__, in.path);
    gkfs::metadata::Metadata md(in.mode);
    try {
        // create metadentry
        gkfs::metadata::create(in.path, md);
        out.err = 0;
    } catch (const std::exception& e) {
        GKFS_DATA->spdlogger()->error("{}() Failed to create metadentry: '{}'", __func__, e.what());
        out.err = -1;
    }
    GKFS_DATA->spdlogger()->debug("{}() Sending output err '{}'", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        GKFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}


hg_return_t rpc_srv_stat(hg_handle_t handle) {
    rpc_path_only_in_t in{};
    rpc_stat_out_t out{};
    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        GKFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    GKFS_DATA->spdlogger()->debug("{}() path: '{}'", __func__, in.path);
    std::string val;

    try {
        // get the metadata
        val = gkfs::metadata::get_str(in.path);
        out.db_val = val.c_str();
        out.err = 0;
        GKFS_DATA->spdlogger()->debug("{}() Sending output mode '{}'", __func__, out.db_val);
    } catch (const gkfs::metadata::NotFoundException& e) {
        GKFS_DATA->spdlogger()->debug("{}() Entry not found: '{}'", __func__, in.path);
        out.err = ENOENT;
    } catch (const std::exception& e) {
        GKFS_DATA->spdlogger()->error("{}() Failed to get metadentry from DB: '{}'", __func__, e.what());
        out.err = EBUSY;
    }

    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        GKFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}


hg_return_t rpc_srv_decr_size(hg_handle_t handle) {
    rpc_trunc_in_t in{};
    rpc_err_out_t out{};

    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS) {
        GKFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
        throw runtime_error("Failed to retrieve input from handle");
    }

    GKFS_DATA->spdlogger()->debug("{}() path: '{}', length: '{}'", __func__, in.path, in.length);

    try {
        GKFS_DATA->mdb()->decrease_size(in.path, in.length);
        out.err = 0;
    } catch (const std::exception& e) {
        GKFS_DATA->spdlogger()->error("{}() Failed to decrease size: '{}'", __func__, e.what());
        out.err = EIO;
    }

    GKFS_DATA->spdlogger()->debug("{}() Sending output '{}'", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        GKFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
        throw runtime_error("Failed to respond");
    }
    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}


hg_return_t rpc_srv_remove(hg_handle_t handle) {
    rpc_rm_node_in_t in{};
    rpc_err_out_t out{};

    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        GKFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    GKFS_DATA->spdlogger()->debug("{}() Got remove node RPC with path '{}'", __func__, in.path);

    // Remove metadentry if exists on the node and remove all chunks for that file
    try {
        gkfs::metadata::remove(in.path);
        out.err = 0;
    } catch (const gkfs::metadata::DBException& e) {
        GKFS_DATA->spdlogger()->error("{}(): path '{}' message '{}'", __func__, in.path, e.what());
        out.err = EIO;
    } catch (const gkfs::data::ChunkStorageException& e) {
        GKFS_DATA->spdlogger()->error("{}(): path '{}' errcode '{}' message '{}'", __func__, in.path, e.code().value(),
                                      e.what());
        out.err = e.code().value();
    } catch (const std::exception& e) {
        GKFS_DATA->spdlogger()->error("{}() path '{}' message '{}'", __func__, in.path, e.what());
        out.err = EBUSY;
    }

    GKFS_DATA->spdlogger()->debug("{}() Sending output '{}'", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        GKFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }
    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}


hg_return_t rpc_srv_update_metadentry(hg_handle_t handle) {
    rpc_update_metadentry_in_t in{};
    rpc_err_out_t out{};


    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        GKFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    GKFS_DATA->spdlogger()->debug("{}() Got update metadentry RPC with path '{}'", __func__, in.path);

    // do update
    try {
        gkfs::metadata::Metadata md = gkfs::metadata::get(in.path);
        if (in.block_flag == HG_TRUE)
            md.blocks(in.blocks);
        if (in.nlink_flag == HG_TRUE)
            md.link_count(in.nlink);
        if (in.size_flag == HG_TRUE)
            md.size(in.size);
        if (in.atime_flag == HG_TRUE)
            md.atime(in.atime);
        if (in.mtime_flag == HG_TRUE)
            md.mtime(in.mtime);
        if (in.ctime_flag == HG_TRUE)
            md.ctime(in.ctime);
        gkfs::metadata::update(in.path, md);
        out.err = 0;
    } catch (const std::exception& e) {
        //TODO handle NotFoundException
        GKFS_DATA->spdlogger()->error("{}() Failed to update entry", __func__);
        out.err = 1;
    }

    GKFS_DATA->spdlogger()->debug("{}() Sending output '{}'", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        GKFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}


hg_return_t rpc_srv_update_metadentry_size(hg_handle_t handle) {
    rpc_update_metadentry_size_in_t in{};
    rpc_update_metadentry_size_out_t out{};


    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        GKFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    GKFS_DATA->spdlogger()->debug("{}() path: '{}', size: '{}', offset: '{}', append: '{}'", __func__, in.path, in.size,
                                  in.offset, in.append);

    try {
        gkfs::metadata::update_size(in.path, in.size, in.offset, (in.append == HG_TRUE));
        out.err = 0;
        //TODO the actual size of the file could be different after the size update
        // do to concurrency on size
        out.ret_size = in.size + in.offset;
    } catch (const gkfs::metadata::NotFoundException& e) {
        GKFS_DATA->spdlogger()->debug("{}() Entry not found: '{}'", __func__, in.path);
        out.err = ENOENT;
    } catch (const std::exception& e) {
        GKFS_DATA->spdlogger()->error("{}() Failed to update metadentry size on DB: '{}'", __func__, e.what());
        out.err = EBUSY;
    }

    GKFS_DATA->spdlogger()->debug("{}() Sending output '{}'", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        GKFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}


hg_return_t rpc_srv_get_metadentry_size(hg_handle_t handle) {
    rpc_path_only_in_t in{};
    rpc_get_metadentry_size_out_t out{};


    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        GKFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    GKFS_DATA->spdlogger()->debug("{}() Got update metadentry size RPC with path '{}'", __func__, in.path);

    // do update
    try {
        out.ret_size = gkfs::metadata::get_size(in.path);
        out.err = 0;
    } catch (const gkfs::metadata::NotFoundException& e) {
        GKFS_DATA->spdlogger()->debug("{}() Entry not found: '{}'", __func__, in.path);
        out.err = ENOENT;
    } catch (const std::exception& e) {
        GKFS_DATA->spdlogger()->error("{}() Failed to get metadentry size from DB: '{}'", __func__, e.what());
        out.err = EBUSY;
    }

    GKFS_DATA->spdlogger()->debug("{}() Sending output '{}'", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        GKFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}


hg_return_t rpc_srv_get_dirents(hg_handle_t handle) {
    rpc_get_dirents_in_t in{};
    rpc_get_dirents_out_t out{};
    out.err = EIO;
    out.dirents_size = 0;
    hg_bulk_t bulk_handle = nullptr;

    // Get input parmeters
    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS) {
        GKFS_DATA->spdlogger()->error(
                "{}() Could not get RPC input data with err '{}'", __func__, ret);
        out.err = EBUSY;
        return gkfs::rpc::cleanup_respond(&handle, &in, &out);
    }

    // Retrieve size of source buffer
    auto hgi = margo_get_info(handle);
    auto mid = margo_hg_info_get_instance(hgi);
    auto bulk_size = margo_bulk_get_size(in.bulk_handle);
    GKFS_DATA->spdlogger()->debug("{}() Got RPC: path '{}' bulk_size '{}' ", __func__, in.path, bulk_size);

    //Get directory entries from local DB
    vector<pair<string, bool>> entries{};
    try {
        entries = gkfs::metadata::get_dirents(in.path);
    } catch (const ::exception& e) {
        GKFS_DATA->spdlogger()->error("{}() Error during get_dirents(): '{}'", __func__, e.what());
        return gkfs::rpc::cleanup_respond(&handle, &in, &out);
    }

    GKFS_DATA->spdlogger()->trace("{}() path '{}' Read database with '{}' entries", __func__, in.path,
                                  entries.size());

    if (entries.empty()) {
        out.err = 0;
        return gkfs::rpc::cleanup_respond(&handle, &in, &out);
    }

    //Calculate total output size
    //TODO OPTIMIZATION: this can be calculated inside db_get_dirents
    size_t tot_names_size = 0;
    for (auto const& e: entries) {
        tot_names_size += e.first.size();
    }

    // tot_names_size (# characters in entry) + # entries * (bool size + char size for \0 character)
    size_t out_size = tot_names_size + entries.size() * (sizeof(bool) + sizeof(char));
    if (bulk_size < out_size) {
        //Source buffer is smaller than total output size
        GKFS_DATA->spdlogger()->error(
                "{}() Entries do not fit source buffer. bulk_size '{}' < out_size '{}' must be satisfied!", __func__,
                bulk_size, out_size);
        out.err = ENOBUFS;
        return gkfs::rpc::cleanup_respond(&handle, &in, &out);
    }

    void* bulk_buf; //buffer for bulk transfer
    // create bulk handle and allocated memory for buffer with out_size information
    ret = margo_bulk_create(mid, 1, nullptr, &out_size, HG_BULK_READ_ONLY, &bulk_handle);
    if (ret != HG_SUCCESS) {
        GKFS_DATA->spdlogger()->error("{}() Failed to create bulk handle", __func__);
        return gkfs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    }
    // access the internally allocated memory buffer and put it into bulk_buf
    uint32_t actual_count; // number of segments. we use one here because we push the whole buffer at once
    ret = margo_bulk_access(bulk_handle, 0, out_size, HG_BULK_READ_ONLY, 1, &bulk_buf,
                            &out_size, &actual_count);
    if (ret != HG_SUCCESS || actual_count != 1) {
        GKFS_DATA->spdlogger()->error("{}() Failed to access allocated buffer from bulk handle", __func__);
        return gkfs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    }

    GKFS_DATA->spdlogger()->trace(
            "{}() path '{}' entries '{}' out_size '{}'. Set up local read only bulk handle and allocated buffer with size '{}'",
            __func__, in.path, entries.size(), out_size, out_size);

    //Serialize output data on local buffer
    auto out_buff_ptr = static_cast<char*>(bulk_buf);
    auto bool_ptr = reinterpret_cast<bool*>(out_buff_ptr);
    auto names_ptr = out_buff_ptr + entries.size();

    for (auto const& e: entries) {
        if (e.first.empty()) {
            GKFS_DATA->spdlogger()->warn(
                    "{}() Entry in readdir() empty. If this shows up, something else is very wrong.", __func__);
        }
        *bool_ptr = e.second;
        bool_ptr++;

        const auto name = e.first.c_str();
        ::strcpy(names_ptr, name);
        // number of characters + \0 terminator
        names_ptr += e.first.size() + 1;
    }

    GKFS_DATA->spdlogger()->trace(
            "{}() path '{}' entries '{}' out_size '{}'. Copied data to bulk_buffer. NEXT bulk_transfer", __func__,
            in.path, entries.size(), out_size);

    ret = margo_bulk_transfer(mid, HG_BULK_PUSH, hgi->addr, in.bulk_handle, 0, bulk_handle, 0, out_size);
    if (ret != HG_SUCCESS) {
        GKFS_DATA->spdlogger()->error(
                "{}() Failed to push '{}' dirents on path '{}' to client with bulk size '{}' and out_size '{}'",
                __func__,
                entries.size(), in.path, bulk_size, out_size);
        out.err = EBUSY;
        return gkfs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    }

    out.dirents_size = entries.size();
    out.err = 0;
    GKFS_DATA->spdlogger()->debug("{}() Sending output response err '{}' dirents_size '{}'. DONE", __func__,
                                  out.err,
                                  out.dirents_size);
    return gkfs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
}


#ifdef HAS_SYMLINKS

hg_return_t rpc_srv_mk_symlink(hg_handle_t handle) {
    rpc_mk_symlink_in_t in{};
    rpc_err_out_t out{};

    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS) {
        GKFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    }
    GKFS_DATA->spdlogger()->debug("{}() Got RPC with path '{}'", __func__, in.path);

    try {
        gkfs::metadata::Metadata md = {gkfs::metadata::LINK_MODE, in.target_path};
        // create metadentry
        gkfs::metadata::create(in.path, md);
        out.err = 0;
    } catch (const std::exception& e) {
        GKFS_DATA->spdlogger()->error("{}() Failed to create metadentry: '{}'", __func__, e.what());
        out.err = -1;
    }
    GKFS_DATA->spdlogger()->debug("{}() Sending output err '{}'", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        GKFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

#endif

}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_create)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_stat)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_decr_size)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_remove)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_update_metadentry)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_update_metadentry_size)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_get_metadentry_size)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_get_dirents)

#ifdef HAS_SYMLINKS

DEFINE_MARGO_RPC_HANDLER(rpc_srv_mk_symlink)

#endif