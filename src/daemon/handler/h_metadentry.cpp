/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#include <global/rpc/rpc_types.hpp>
#include <global/rpc/rpc_utils.hpp>
#include <daemon/handler/rpc_defs.hpp>
#include <daemon/backend/metadata/db.hpp>

#include <daemon/ops/metadentry.hpp>

using namespace std;

static hg_return_t rpc_srv_mk_node(hg_handle_t handle) {
    rpc_mk_node_in_t in;
    rpc_err_out_t out;

    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        ADAFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    ADAFS_DATA->spdlogger()->debug("{}() Got RPC with path '{}'", __func__, in.path);
    Metadata md(in.mode);
    try {
        // create metadentry
        create_metadentry(in.path, md);
        out.err = 0;
    } catch (const std::exception& e) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to create metadentry: {}",  __func__, e.what());
        out.err = -1;
    }
    ADAFS_DATA->spdlogger()->debug("{}() Sending output err {}", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_mk_node)

static hg_return_t rpc_srv_stat(hg_handle_t handle) {
    rpc_path_only_in_t in{};
    rpc_stat_out_t out{};
    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        ADAFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    ADAFS_DATA->spdlogger()->debug("{}() path: '{}'", __func__, in.path);
    std::string val;

    try {
        // get the metadata
        val = get_metadentry_str(in.path);
        out.db_val = val.c_str();
        out.err = 0;
        ADAFS_DATA->spdlogger()->debug("{}() Sending output mode '{}'", __func__, out.db_val);
    } catch (const NotFoundException& e) {
        ADAFS_DATA->spdlogger()->debug("{}() Entry not found: '{}'", __func__, in.path);
        out.err = ENOENT;
    } catch (const std::exception& e) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to get metadentry from DB: '{}'", __func__, e.what());
        out.err = EBUSY;
    }

    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_stat)

static hg_return_t rpc_srv_decr_size(hg_handle_t handle) {
    rpc_trunc_in_t in{};
    rpc_err_out_t out{};

    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
        throw runtime_error("Failed to retrieve input from handle");
    }

    ADAFS_DATA->spdlogger()->debug("{}() path: '{}', length: {}", __func__, in.path, in.length);

    try {
        ADAFS_DATA->mdb()->decrease_size(in.path, in.length);
        out.err = 0;
    } catch (const std::exception& e) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to decrease size: {}", __func__, e.what());
        out.err = EIO;
    }

    ADAFS_DATA->spdlogger()->debug("{}() Sending output {}", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
        throw runtime_error("Failed to respond");
    }
    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_decr_size)

static hg_return_t rpc_srv_rm_node(hg_handle_t handle) {
    rpc_rm_node_in_t in{};
    rpc_err_out_t out{};

    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        ADAFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    ADAFS_DATA->spdlogger()->debug("{}() Got remove node RPC with path '{}'", __func__, in.path);

    try {
        // Remove metadentry if exists on the node
        // and remove all chunks for that file
        remove_node(in.path);
        out.err = 0;
    } catch (const NotFoundException& e) {
        /* The metadentry was not found on this node,
         * this is not an error. At least one node involved in this
         * broadcast operation will find and delete the entry on its local
         * MetadataDB.
         * TODO: send the metadentry remove only to the node that actually
         * has it.
         */
        out.err = 0;
    } catch (const std::exception& e) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to remove node: {}", __func__, e.what());
        out.err = EBUSY;
    }

    ADAFS_DATA->spdlogger()->debug("{}() Sending output {}", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }
    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_rm_node)


static hg_return_t rpc_srv_update_metadentry(hg_handle_t handle) {
    rpc_update_metadentry_in_t in{};
    rpc_err_out_t out{};


    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        ADAFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    ADAFS_DATA->spdlogger()->debug("{}() Got update metadentry RPC with path '{}'", __func__, in.path);

    // do update
    try {
        Metadata md = get_metadentry(in.path);
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
        update_metadentry(in.path, md);
        out.err = 0;
    } catch (const std::exception& e){
        //TODO handle NotFoundException
        ADAFS_DATA->spdlogger()->error("{}() Failed to update entry", __func__);
        out.err = 1;
    }

    ADAFS_DATA->spdlogger()->debug("{}() Sending output {}", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_update_metadentry)

static hg_return_t rpc_srv_update_metadentry_size(hg_handle_t handle) {
    rpc_update_metadentry_size_in_t in{};
    rpc_update_metadentry_size_out_t out{};


    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        ADAFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    ADAFS_DATA->spdlogger()->debug("{}() path: {}, size: {}, offset: {}, append: {}", __func__, in.path, in.size, in.offset, in.append);

    try {
        update_metadentry_size(in.path, in.size, in.offset, (in.append == HG_TRUE));
        out.err = 0;
        //TODO the actual size of the file could be different after the size update
        // do to concurrency on size
        out.ret_size = in.size + in.offset;
    } catch (const NotFoundException& e) {
        ADAFS_DATA->spdlogger()->debug("{}() Entry not found: '{}'", __func__, in.path);
        out.err = ENOENT;
    } catch (const std::exception& e) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to update metadentry size on DB: {}", __func__, e.what());
        out.err = EBUSY;
    }

    ADAFS_DATA->spdlogger()->debug("{}() Sending output {}", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_update_metadentry_size)

static hg_return_t rpc_srv_get_metadentry_size(hg_handle_t handle) {
    rpc_path_only_in_t in{};
    rpc_get_metadentry_size_out_t out{};


    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS)
        ADAFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    assert(ret == HG_SUCCESS);
    ADAFS_DATA->spdlogger()->debug("{}() Got update metadentry size RPC with path {}", __func__, in.path);

    // do update
    try {
        out.ret_size = get_metadentry_size(in.path);
        out.err = 0;
    } catch (const NotFoundException& e) {
        ADAFS_DATA->spdlogger()->debug("{}() Entry not found: {}", __func__, in.path);
        out.err = ENOENT;
    } catch (const std::exception& e) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to get metadentry size from DB: {}", __func__, e.what());
        out.err = EBUSY;
    }

    ADAFS_DATA->spdlogger()->debug("{}() Sending output {}", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_get_metadentry_size)

static hg_return_t rpc_srv_get_dirents(hg_handle_t handle) {
    rpc_get_dirents_in_t in{};
    rpc_get_dirents_out_t out{};
    hg_bulk_t bulk_handle = nullptr;

    // Get input parmeters
    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error(
                "{}() Could not get RPC input data with err {}", __func__, ret);
        return ret;
    }

    // Retrieve size of source buffer
    auto hgi = margo_get_info(handle);
    auto mid = margo_hg_info_get_instance(hgi);
    ADAFS_DATA->spdlogger()->debug(
            "{}() Got dirents RPC with path {}", __func__, in.path);
    auto bulk_size = margo_bulk_get_size(in.bulk_handle);

    //Get directory entries from local DB
    std::vector<std::pair<std::string, bool>> entries = get_dirents(in.path);

    out.dirents_size = entries.size();

    if(entries.size() == 0){
        out.err = 0;
        return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
    }

    //Calculate total output size
    //TODO OPTIMIZATION: this can be calculated inside db_get_dirents
    size_t tot_names_size = 0;
    for(auto const& e: entries){
        tot_names_size += e.first.size();
    }

    size_t out_size = tot_names_size + entries.size() * ( sizeof(bool) + sizeof(char) );
    if(bulk_size < out_size){
        //Source buffer is smaller than total output size
        ADAFS_DATA->spdlogger()->error("{}() Entries do not fit source buffer", __func__);
        out.err = ENOBUFS;
        return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
    }

    //Serialize output data on local buffer
    auto out_buff = std::make_unique<char[]>(out_size);
    char * out_buff_ptr = out_buff.get();

    auto bool_ptr = reinterpret_cast<bool*>(out_buff_ptr);
    char* names_ptr = out_buff_ptr + entries.size();
    for(auto const& e: entries){
        *bool_ptr = e.second;
        bool_ptr++;

        const char* name = e.first.c_str();
        std::strcpy(names_ptr, name);
        names_ptr += e.first.size() + 1;
    }

    ret = margo_bulk_create(mid, 1, reinterpret_cast<void**>(&out_buff_ptr), &out_size, HG_BULK_READ_ONLY, &bulk_handle);
    if (ret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to create bulk handle", __func__);
        out.err = EBUSY;
        return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
    }

    ret = margo_bulk_transfer(mid, HG_BULK_PUSH, hgi->addr,
                              in.bulk_handle, 0,
                              bulk_handle, 0,
                              out_size);
    if (ret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error(
                "{}() Failed push dirents on path {} to client",
                __func__, in.path
                );
        return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
    }

    out.dirents_size = entries.size();
    out.err = 0;
    ADAFS_DATA->spdlogger()->debug(
            "{}() Sending output response", __func__);
    return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_get_dirents)

#ifdef HAS_SYMLINKS

static hg_return_t rpc_srv_mk_symlink(hg_handle_t handle) {
    rpc_mk_symlink_in_t in{};
    rpc_err_out_t out{};

    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to retrieve input from handle", __func__);
    }
    ADAFS_DATA->spdlogger()->debug("{}() Got RPC with path '{}'", __func__, in.path);

    try {
        Metadata md = {LINK_MODE, in.target_path};
        // create metadentry
        create_metadentry(in.path, md);
        out.err = 0;
    } catch (const std::exception& e) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to create metadentry: {}",  __func__, e.what());
        out.err = -1;
    }
    ADAFS_DATA->spdlogger()->debug("{}() Sending output err {}", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }

    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_mk_symlink)

#endif
