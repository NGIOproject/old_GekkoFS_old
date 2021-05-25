/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GKFS_RPCS_TYPES_HPP
#define GKFS_RPCS_TYPES_HPP

// C includes
#include <mercury.h>
#include <mercury_proc_string.h>
#include <mercury_macros.h>

// C++ includes
#include <string>

// hermes includes
#include <hermes.hpp>

#ifndef HG_GEN_PROC_NAME
#define HG_GEN_PROC_NAME(struct_type_name) \
    hg_proc_ ## struct_type_name
#endif


#include <global/global_defs.hpp>
#include <global/rpc/rpc_types.hpp>

namespace hermes {
namespace detail {

struct hg_void_t {
};

static HG_INLINE hg_return_t
hg_proc_void_t(hg_proc_t proc, void* data) {
    (void) proc;
    (void) data;

    return HG_SUCCESS;
}

}
} // namespace hermes::detail

namespace gkfs {
namespace rpc {

//==============================================================================
// definitions for fs_config
struct fs_config {

    // forward declarations of public input/output types for this RPC
    class input;

    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = fs_config;
    using handle_type = hermes::rpc_handle<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = hermes::detail::hg_void_t;
    using mercury_output_type = rpc_config_out_t;

    // RPC public identifier
    // (N.B: we reuse the same IDs assigned by Margo so that the daemon
    // understands Hermes RPCs)
    constexpr static const uint64_t public_id = 3033006080;

    // RPC internal Mercury identifier
    constexpr static const hg_id_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = gkfs::rpc::tag::fs_config;

    // requires response?
    constexpr static const auto requires_response = true;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb =
            hermes::detail::hg_proc_void_t;

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb =
            HG_GEN_PROC_NAME(rpc_config_out_t);

    class input {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input() {}

        input(input&& rhs) = default;

        input(const input& other) = default;

        input& operator=(input&& rhs) = default;

        input& operator=(const input& other) = default;

        explicit
        input(const hermes::detail::hg_void_t& other) {}

        explicit
        operator hermes::detail::hg_void_t() {
            return {};
        }
    };

    class output {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        output() :
                m_mountdir(),
                m_rootdir(),
                m_atime_state(),
                m_mtime_state(),
                m_ctime_state(),
                m_link_cnt_state(),
                m_blocks_state(),
                m_uid(),
                m_gid() {}

        output(const std::string& mountdir,
               const std::string& rootdir,
               bool atime_state,
               bool mtime_state,
               bool ctime_state,
               bool link_cnt_state,
               bool blocks_state,
               uint32_t uid,
               uint32_t gid) :
                m_mountdir(mountdir),
                m_rootdir(rootdir),
                m_atime_state(atime_state),
                m_mtime_state(mtime_state),
                m_ctime_state(ctime_state),
                m_link_cnt_state(link_cnt_state),
                m_blocks_state(blocks_state),
                m_uid(uid),
                m_gid(gid) {}

        output(output&& rhs) = default;

        output(const output& other) = default;

        output& operator=(output&& rhs) = default;

        output& operator=(const output& other) = default;

        explicit
        output(const rpc_config_out_t& out) {

            if (out.mountdir != nullptr) {
                m_mountdir = out.mountdir;
            }

            if (out.rootdir != nullptr) {
                m_rootdir = out.rootdir;
            }

            m_atime_state = out.atime_state;
            m_mtime_state = out.mtime_state;
            m_ctime_state = out.ctime_state;
            m_link_cnt_state = out.link_cnt_state;
            m_blocks_state = out.blocks_state;
            m_uid = out.uid;
            m_gid = out.gid;
        }

        std::string
        mountdir() const {
            return m_mountdir;
        }

        std::string
        rootdir() const {
            return m_rootdir;
        }

        bool
        atime_state() const {
            return m_atime_state;
        }

        bool
        mtime_state() const {
            return m_mtime_state;
        }

        bool
        ctime_state() const {
            return m_ctime_state;
        }

        bool
        link_cnt_state() const {
            return m_link_cnt_state;
        }

        bool
        blocks_state() const {
            return m_blocks_state;
        }

        uint32_t
        uid() const {
            return m_uid;
        }

        uint32_t
        gid() const {
            return m_gid;
        }

    private:
        std::string m_mountdir;
        std::string m_rootdir;
        bool m_atime_state;
        bool m_mtime_state;
        bool m_ctime_state;
        bool m_link_cnt_state;
        bool m_blocks_state;
        uint32_t m_uid;
        uint32_t m_gid;
    };
};


//==============================================================================
// definitions for create
struct create {

    // forward declarations of public input/output types for this RPC
    class input;

    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = create;
    using handle_type = hermes::rpc_handle<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = rpc_mk_node_in_t;
    using mercury_output_type = rpc_err_out_t;

    // RPC public identifier
    // (N.B: we reuse the same IDs assigned by Margo so that the daemon
    // understands Hermes RPCs)
    constexpr static const uint64_t public_id = 796590080;

    // RPC internal Mercury identifier
    constexpr static const hg_id_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = gkfs::rpc::tag::create;

    // requires response?
    constexpr static const auto requires_response = true;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb =
            HG_GEN_PROC_NAME(rpc_mk_node_in_t);

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb =
            HG_GEN_PROC_NAME(rpc_err_out_t);

    class input {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input(const std::string& path,
              uint32_t mode) :
                m_path(path),
                m_mode(mode) {}

        input(input&& rhs) = default;

        input(const input& other) = default;

        input& operator=(input&& rhs) = default;

        input& operator=(const input& other) = default;

        std::string
        path() const {
            return m_path;
        }

        uint32_t
        mode() const {
            return m_mode;
        }

        explicit
        input(const rpc_mk_node_in_t& other) :
                m_path(other.path),
                m_mode(other.mode) {}

        explicit
        operator rpc_mk_node_in_t() {
            return {m_path.c_str(), m_mode};
        }

    private:
        std::string m_path;
        uint32_t m_mode;
    };

    class output {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        output() :
                m_err() {}

        output(int32_t err) :
                m_err(err) {}

        output(output&& rhs) = default;

        output(const output& other) = default;

        output& operator=(output&& rhs) = default;

        output& operator=(const output& other) = default;

        explicit
        output(const rpc_err_out_t& out) {
            m_err = out.err;
        }

        int32_t
        err() const {
            return m_err;
        }

    private:
        int32_t m_err;
    };
};

//==============================================================================
// definitions for stat
struct stat {

    // forward declarations of public input/output types for this RPC
    class input;

    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = stat;
    using handle_type = hermes::rpc_handle<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = rpc_path_only_in_t;
    using mercury_output_type = rpc_stat_out_t;

    // RPC public identifier
    // (N.B: we reuse the same IDs assigned by Margo so that the daemon
    // understands Hermes RPCs)
    constexpr static const uint64_t public_id = 1396244480;

    // RPC internal Mercury identifier
    constexpr static const hg_id_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = gkfs::rpc::tag::stat;

    // requires response?
    constexpr static const auto requires_response = true;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb =
            HG_GEN_PROC_NAME(rpc_path_only_in_t);

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb =
            HG_GEN_PROC_NAME(rpc_stat_out_t);

    class input {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input(const std::string& path) :
                m_path(path) {}

        input(input&& rhs) = default;

        input(const input& other) = default;

        input& operator=(input&& rhs) = default;

        input& operator=(const input& other) = default;

        std::string
        path() const {
            return m_path;
        }

        explicit
        input(const rpc_path_only_in_t& other) :
                m_path(other.path) {}

        explicit
        operator rpc_path_only_in_t() {
            return {m_path.c_str()};
        }

    private:
        std::string m_path;
    };

    class output {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        output() :
                m_err(),
                m_db_val() {}

        output(int32_t err, const std::string& db_val) :
                m_err(err),
                m_db_val(db_val) {}

        output(output&& rhs) = default;

        output(const output& other) = default;

        output& operator=(output&& rhs) = default;

        output& operator=(const output& other) = default;

        explicit
        output(const rpc_stat_out_t& out) {
            m_err = out.err;

            if (out.db_val != nullptr) {
                m_db_val = out.db_val;
            }
        }

        int32_t
        err() const {
            return m_err;
        }

        std::string
        db_val() const {
            return m_db_val;
        }

    private:
        int32_t m_err;
        std::string m_db_val;
    };
};

//==============================================================================
// definitions for remove
struct remove {

    // forward declarations of public input/output types for this RPC
    class input;

    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = remove;
    using handle_type = hermes::rpc_handle<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = rpc_rm_node_in_t;
    using mercury_output_type = rpc_err_out_t;

    // RPC public identifier
    // (N.B: we reuse the same IDs assigned by Margo so that the daemon
    // understands Hermes RPCs)
    constexpr static const uint64_t public_id = 2549415936;

    // RPC internal Mercury identifier
    constexpr static const hg_id_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = gkfs::rpc::tag::remove;

    // requires response?
    constexpr static const auto requires_response = true;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb =
            HG_GEN_PROC_NAME(rpc_rm_node_in_t);

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb =
            HG_GEN_PROC_NAME(rpc_err_out_t);

    class input {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input(const std::string& path) :
                m_path(path) {}

        input(input&& rhs) = default;

        input(const input& other) = default;

        input& operator=(input&& rhs) = default;

        input& operator=(const input& other) = default;

        std::string
        path() const {
            return m_path;
        }

        explicit
        input(const rpc_rm_node_in_t& other) :
                m_path(other.path) {}

        explicit
        operator rpc_rm_node_in_t() {
            return {m_path.c_str()};
        }

    private:
        std::string m_path;
    };

    class output {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        output() :
                m_err() {}

        output(int32_t err) :
                m_err(err) {}

        output(output&& rhs) = default;

        output(const output& other) = default;

        output& operator=(output&& rhs) = default;

        output& operator=(const output& other) = default;

        explicit
        output(const rpc_err_out_t& out) {
            m_err = out.err;
        }

        int32_t
        err() const {
            return m_err;
        }

    private:
        int32_t m_err;
    };
};

//==============================================================================
// definitions for decr_size
struct decr_size {

    // forward declarations of public input/output types for this RPC
    class input;

    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = decr_size;
    using handle_type = hermes::rpc_handle<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = rpc_trunc_in_t;
    using mercury_output_type = rpc_err_out_t;

    // RPC public identifier
    // (N.B: we reuse the same IDs assigned by Margo so that the daemon
    // understands Hermes RPCs)
    constexpr static const uint64_t public_id = 1291649024;

    // RPC internal Mercury identifier
    constexpr static const hg_id_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = gkfs::rpc::tag::decr_size;

    // requires response?
    constexpr static const auto requires_response = true;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb =
            HG_GEN_PROC_NAME(rpc_trunc_in_t);

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb =
            HG_GEN_PROC_NAME(rpc_err_out_t);

    class input {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input(const std::string& path, uint64_t length) :
                m_path(path),
                m_length(length) {}

        input(input&& rhs) = default;

        input(const input& other) = default;

        input& operator=(input&& rhs) = default;

        input& operator=(const input& other) = default;

        std::string
        path() const {
            return m_path;
        }

        uint64_t
        length() const {
            return m_length;
        }

        explicit
        input(const rpc_trunc_in_t& other) :
                m_path(other.path),
                m_length(other.length) {}

        explicit
        operator rpc_trunc_in_t() {
            return {m_path.c_str(), m_length};
        }

    private:
        std::string m_path;
        uint64_t m_length;
    };

    class output {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        output() :
                m_err() {}

        output(int32_t err) :
                m_err(err) {}

        output(output&& rhs) = default;

        output(const output& other) = default;

        output& operator=(output&& rhs) = default;

        output& operator=(const output& other) = default;

        explicit
        output(const rpc_err_out_t& out) {
            m_err = out.err;
        }

        int32_t
        err() const {
            return m_err;
        }

    private:
        int32_t m_err;
    };
};

//==============================================================================
// definitions for update_metadentry
struct update_metadentry {

    // forward declarations of public input/output types for this RPC
    class input;

    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = update_metadentry;
    using handle_type = hermes::rpc_handle<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = rpc_update_metadentry_in_t;
    using mercury_output_type = rpc_err_out_t;

    // RPC public identifier
    // (N.B: we reuse the same IDs assigned by Margo so that the daemon
    // understands Hermes RPCs)
    constexpr static const uint64_t public_id = 99483648;

    // RPC internal Mercury identifier
    constexpr static const hg_id_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = gkfs::rpc::tag::update_metadentry;

    // requires response?
    constexpr static const auto requires_response = true;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb =
            HG_GEN_PROC_NAME(rpc_update_metadentry_in_t);

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb =
            HG_GEN_PROC_NAME(rpc_err_out_t);

    class input {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input(const std::string& path,
              uint64_t nlink,
              uint32_t mode,
              uint32_t uid,
              uint32_t gid,
              int64_t size,
              int64_t blocks,
              int64_t atime,
              int64_t mtime,
              int64_t ctime,
              bool nlink_flag,
              bool mode_flag,
              bool size_flag,
              bool block_flag,
              bool atime_flag,
              bool mtime_flag,
              bool ctime_flag) :
                m_path(path),
                m_nlink(nlink),
                m_mode(mode),
                m_uid(uid),
                m_gid(gid),
                m_size(size),
                m_blocks(blocks),
                m_atime(atime),
                m_mtime(mtime),
                m_ctime(ctime),
                m_nlink_flag(nlink_flag),
                m_mode_flag(mode_flag),
                m_size_flag(size_flag),
                m_block_flag(block_flag),
                m_atime_flag(atime_flag),
                m_mtime_flag(mtime_flag),
                m_ctime_flag(ctime_flag) {}

        input(input&& rhs) = default;

        input(const input& other) = default;

        input& operator=(input&& rhs) = default;

        input& operator=(const input& other) = default;

        std::string
        path() const {
            return m_path;
        }

        uint64_t
        nlink() const {
            return m_nlink;
        }

        uint32_t
        mode() const {
            return m_mode;
        }

        uint32_t
        uid() const {
            return m_uid;
        }

        uint32_t
        gid() const {
            return m_gid;
        }

        int64_t
        size() const {
            return m_size;
        }

        int64_t
        blocks() const {
            return m_blocks;
        }

        int64_t
        atime() const {
            return m_atime;
        }

        int64_t
        mtime() const {
            return m_mtime;
        }

        int64_t
        ctime() const {
            return m_ctime;
        }

        bool
        nlink_flag() const {
            return m_nlink_flag;
        }

        bool
        mode_flag() const {
            return m_mode_flag;
        }

        bool
        size_flag() const {
            return m_size_flag;
        }

        bool
        block_flag() const {
            return m_block_flag;
        }

        bool
        atime_flag() const {
            return m_atime_flag;
        }

        bool
        mtime_flag() const {
            return m_mtime_flag;
        }

        bool
        ctime_flag() const {
            return m_ctime_flag;
        }

        explicit
        input(const rpc_update_metadentry_in_t& other) :
                m_path(other.path),
                m_nlink(other.nlink),
                m_mode(other.mode),
                m_uid(other.uid),
                m_gid(other.gid),
                m_size(other.size),
                m_blocks(other.blocks),
                m_atime(other.atime),
                m_mtime(other.mtime),
                m_ctime(other.ctime),
                m_nlink_flag(other.nlink_flag),
                m_mode_flag(other.mode_flag),
                m_size_flag(other.size_flag),
                m_block_flag(other.block_flag),
                m_atime_flag(other.atime_flag),
                m_mtime_flag(other.mtime_flag),
                m_ctime_flag(other.ctime_flag) {}

        explicit
        operator rpc_update_metadentry_in_t() {
            return {m_path.c_str(),
                    m_nlink,
                    m_mode,
                    m_uid,
                    m_gid,
                    m_size,
                    m_blocks,
                    m_atime,
                    m_mtime,
                    m_ctime,
                    m_nlink_flag,
                    m_mode_flag,
                    m_size_flag,
                    m_block_flag,
                    m_atime_flag,
                    m_mtime_flag,
                    m_ctime_flag};
        }

    private:
        std::string m_path;
        uint64_t m_nlink;
        uint32_t m_mode;
        uint32_t m_uid;
        uint32_t m_gid;
        int64_t m_size;
        int64_t m_blocks;
        int64_t m_atime;
        int64_t m_mtime;
        int64_t m_ctime;
        bool m_nlink_flag;
        bool m_mode_flag;
        bool m_size_flag;
        bool m_block_flag;
        bool m_atime_flag;
        bool m_mtime_flag;
        bool m_ctime_flag;
    };

    class output {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        output() :
                m_err() {}

        output(int32_t err) :
                m_err(err) {}

        output(output&& rhs) = default;

        output(const output& other) = default;

        output& operator=(output&& rhs) = default;

        output& operator=(const output& other) = default;

        explicit
        output(const rpc_err_out_t& out) {
            m_err = out.err;
        }

        int32_t
        err() const {
            return m_err;
        }

    private:
        int32_t m_err;
    };
};

//==============================================================================
// definitions for get_metadentry_size
struct get_metadentry_size {

    // forward declarations of public input/output types for this RPC
    class input;

    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = get_metadentry_size;
    using handle_type = hermes::rpc_handle<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = rpc_path_only_in_t;
    using mercury_output_type = rpc_get_metadentry_size_out_t;

    // RPC public identifier
    // (N.B: we reuse the same IDs assigned by Margo so that the daemon
    // understands Hermes RPCs)
    constexpr static const uint64_t public_id = 3426484224;

    // RPC internal Mercury identifier
    constexpr static const hg_id_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = gkfs::rpc::tag::get_metadentry_size;

    // requires response?
    constexpr static const auto requires_response = true;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb =
            HG_GEN_PROC_NAME(rpc_path_only_in_t);

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb =
            HG_GEN_PROC_NAME(rpc_get_metadentry_size_out_t);

    class input {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input(const std::string& path) :
                m_path(path) {}

        input(input&& rhs) = default;

        input(const input& other) = default;

        input& operator=(input&& rhs) = default;

        input& operator=(const input& other) = default;

        std::string
        path() const {
            return m_path;
        }

        explicit
        input(const rpc_path_only_in_t& other) :
                m_path(other.path) {}

        explicit
        operator rpc_path_only_in_t() {
            return {m_path.c_str()};
        }

    private:
        std::string m_path;
    };

    class output {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        output() :
                m_err(),
                m_ret_size() {}

        output(int32_t err, int64_t ret_size) :
                m_err(err),
                m_ret_size(ret_size) {}

        output(output&& rhs) = default;

        output(const output& other) = default;

        output& operator=(output&& rhs) = default;

        output& operator=(const output& other) = default;

        explicit
        output(const rpc_get_metadentry_size_out_t& out) {
            m_err = out.err;
            m_ret_size = out.ret_size;
        }

        int32_t
        err() const {
            return m_err;
        }

        int64_t
        ret_size() const {
            return m_ret_size;
        }

    private:
        int32_t m_err;
        int64_t m_ret_size;
    };
};

//==============================================================================
// definitions for update_metadentry_size
struct update_metadentry_size {

    // forward declarations of public input/output types for this RPC
    class input;

    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = update_metadentry_size;
    using handle_type = hermes::rpc_handle<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = rpc_update_metadentry_size_in_t;
    using mercury_output_type = rpc_update_metadentry_size_out_t;

    // RPC public identifier
    // (N.B: we reuse the same IDs assigned by Margo so that the daemon
    // understands Hermes RPCs)
    constexpr static const uint64_t public_id = 2760900608;

    // RPC internal Mercury identifier
    constexpr static const hg_id_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = gkfs::rpc::tag::update_metadentry_size;

    // requires response?
    constexpr static const auto requires_response = true;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb =
            HG_GEN_PROC_NAME(rpc_update_metadentry_size_in_t);

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb =
            HG_GEN_PROC_NAME(rpc_update_metadentry_size_out_t);

    class input {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input(const std::string& path,
              uint64_t size,
              int64_t offset,
              bool append) :
                m_path(path),
                m_size(size),
                m_offset(offset),
                m_append(append) {}

        input(input&& rhs) = default;

        input(const input& other) = default;

        input& operator=(input&& rhs) = default;

        input& operator=(const input& other) = default;

        std::string
        path() const {
            return m_path;
        }

        uint64_t
        size() const {
            return m_size;
        }

        int64_t
        offset() const {
            return m_offset;
        }

        bool
        append() const {
            return m_append;
        }

        explicit
        input(const rpc_update_metadentry_size_in_t& other) :
                m_path(other.path),
                m_size(other.size),
                m_offset(other.offset),
                m_append(other.append) {}

        explicit
        operator rpc_update_metadentry_size_in_t() {
            return {m_path.c_str(), m_size, m_offset, m_append};
        }

    private:
        std::string m_path;
        uint64_t m_size;
        int64_t m_offset;
        bool m_append;
    };

    class output {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        output() :
                m_err(),
                m_ret_size() {}

        output(int32_t err, int64_t ret_size) :
                m_err(err),
                m_ret_size(ret_size) {}

        output(output&& rhs) = default;

        output(const output& other) = default;

        output& operator=(output&& rhs) = default;

        output& operator=(const output& other) = default;

        explicit
        output(const rpc_update_metadentry_size_out_t& out) {
            m_err = out.err;
            m_ret_size = out.ret_size;
        }

        int32_t
        err() const {
            return m_err;
        }

        int64_t
        ret_size() const {
            return m_ret_size;
        }

    private:
        int32_t m_err;
        int64_t m_ret_size;
    };
};

#ifdef HAS_SYMLINKS

//==============================================================================
// definitions for mk_symlink
struct mk_symlink {

    // forward declarations of public input/output types for this RPC
    class input;

    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = mk_symlink;
    using handle_type = hermes::rpc_handle<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = rpc_mk_symlink_in_t;
    using mercury_output_type = rpc_err_out_t;

    // RPC public identifier
    // (N.B: we reuse the same IDs assigned by Margo so that the daemon
    // understands Hermes RPCs)
    constexpr static const uint64_t public_id = 3207004160;

    // RPC internal Mercury identifier
    constexpr static const hg_id_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = gkfs::rpc::tag::mk_symlink;

    // requires response?
    constexpr static const auto requires_response = true;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb =
            HG_GEN_PROC_NAME(rpc_mk_symlink_in_t);

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb =
            HG_GEN_PROC_NAME(rpc_err_out_t);

    class input {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input(const std::string& path,
              const std::string& target_path) :
                m_path(path),
                m_target_path(target_path) {}

        input(input&& rhs) = default;

        input(const input& other) = default;

        input& operator=(input&& rhs) = default;

        input& operator=(const input& other) = default;

        std::string
        path() const {
            return m_path;
        }

        std::string
        target_path() const {
            return m_target_path;
        }

        explicit
        input(const rpc_mk_symlink_in_t& other) :
                m_path(other.path),
                m_target_path(other.target_path) {}

        explicit
        operator rpc_mk_symlink_in_t() {
            return {m_path.c_str(), m_target_path.c_str()};
        }

    private:
        std::string m_path;
        std::string m_target_path;
    };

    class output {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        output() :
                m_err() {}

        output(int32_t err) :
                m_err(err) {}

        output(output&& rhs) = default;

        output(const output& other) = default;

        output& operator=(output&& rhs) = default;

        output& operator=(const output& other) = default;

        explicit
        output(const rpc_err_out_t& out) {
            m_err = out.err;
        }

        int32_t
        err() const {
            return m_err;
        }

    private:
        int32_t m_err;
    };
};

#endif // HAS_SYMLINKS

//==============================================================================
// definitions for write_data
struct write_data {

    // forward declarations of public input/output types for this RPC
    class input;

    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = write_data;
    using handle_type = hermes::rpc_handle<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = rpc_write_data_in_t;
    using mercury_output_type = rpc_data_out_t;

    // RPC public identifier
    // (N.B: we reuse the same IDs assigned by Margo so that the daemon
    // understands Hermes RPCs)
    constexpr static const uint64_t public_id = 3716481024;

    // RPC internal Mercury identifier
    constexpr static const hg_id_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = gkfs::rpc::tag::write;

    // requires response?
    constexpr static const auto requires_response = true;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb =
            HG_GEN_PROC_NAME(rpc_write_data_in_t);

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb =
            HG_GEN_PROC_NAME(rpc_data_out_t);

    class input {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input(const std::string& path,
              int64_t offset,
              uint64_t host_id,
              uint64_t host_size,
              uint64_t chunk_n,
              uint64_t chunk_start,
              uint64_t chunk_end,
              uint64_t total_chunk_size,
              const hermes::exposed_memory& buffers) :
                m_path(path),
                m_offset(offset),
                m_host_id(host_id),
                m_host_size(host_size),
                m_chunk_n(chunk_n),
                m_chunk_start(chunk_start),
                m_chunk_end(chunk_end),
                m_total_chunk_size(total_chunk_size),
                m_buffers(buffers) {}

        input(input&& rhs) = default;

        input(const input& other) = default;

        input& operator=(input&& rhs) = default;

        input& operator=(const input& other) = default;

        std::string
        path() const {
            return m_path;
        }

        int64_t
        offset() const {
            return m_offset;
        }

        uint64_t
        host_id() const {
            return m_host_id;
        }

        uint64_t
        host_size() const {
            return m_host_size;
        }

        uint64_t
        chunk_n() const {
            return m_chunk_n;
        }

        uint64_t
        chunk_start() const {
            return m_chunk_start;
        }

        uint64_t
        chunk_end() const {
            return m_chunk_end;
        }

        uint64_t
        total_chunk_size() const {
            return m_total_chunk_size;
        }

        hermes::exposed_memory
        buffers() const {
            return m_buffers;
        }

        explicit
        input(const rpc_write_data_in_t& other) :
                m_path(other.path),
                m_offset(other.offset),
                m_host_id(other.host_id),
                m_host_size(other.host_size),
                m_chunk_n(other.chunk_n),
                m_chunk_start(other.chunk_start),
                m_chunk_end(other.chunk_end),
                m_total_chunk_size(other.total_chunk_size),
                m_buffers(other.bulk_handle) {}

        explicit
        operator rpc_write_data_in_t() {
            return {
                    m_path.c_str(),
                    m_offset,
                    m_host_id,
                    m_host_size,
                    m_chunk_n,
                    m_chunk_start,
                    m_chunk_end,
                    m_total_chunk_size,
                    hg_bulk_t(m_buffers)
            };
        }

    private:
        std::string m_path;
        int64_t m_offset;
        uint64_t m_host_id;
        uint64_t m_host_size;
        uint64_t m_chunk_n;
        uint64_t m_chunk_start;
        uint64_t m_chunk_end;
        uint64_t m_total_chunk_size;
        hermes::exposed_memory m_buffers;
    };

    class output {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        output() :
                m_err(),
                m_io_size() {}

        output(int32_t err, size_t io_size) :
                m_err(err),
                m_io_size(io_size) {}

        output(output&& rhs) = default;

        output(const output& other) = default;

        output& operator=(output&& rhs) = default;

        output& operator=(const output& other) = default;

        explicit
        output(const rpc_data_out_t& out) {
            m_err = out.err;
            m_io_size = out.io_size;
        }

        int32_t
        err() const {
            return m_err;
        }

        int64_t
        io_size() const {
            return m_io_size;
        }

    private:
        int32_t m_err;
        size_t m_io_size;
    };
};

//==============================================================================
// definitions for read_data
struct read_data {

    // forward declarations of public input/output types for this RPC
    class input;

    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = read_data;
    using handle_type = hermes::rpc_handle<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = rpc_read_data_in_t;
    using mercury_output_type = rpc_data_out_t;

    // RPC public identifier
    // (N.B: we reuse the same IDs assigned by Margo so that the daemon
    // understands Hermes RPCs)
    constexpr static const uint64_t public_id = 892207104;

    // RPC internal Mercury identifier
    constexpr static const hg_id_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = gkfs::rpc::tag::read;

    // requires response?
    constexpr static const auto requires_response = true;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb =
            HG_GEN_PROC_NAME(rpc_read_data_in_t);

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb =
            HG_GEN_PROC_NAME(rpc_data_out_t);

    class input {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input(const std::string& path,
              int64_t offset,
              uint64_t host_id,
              uint64_t host_size,
              uint64_t chunk_n,
              uint64_t chunk_start,
              uint64_t chunk_end,
              uint64_t total_chunk_size,
              const hermes::exposed_memory& buffers) :
                m_path(path),
                m_offset(offset),
                m_host_id(host_id),
                m_host_size(host_size),
                m_chunk_n(chunk_n),
                m_chunk_start(chunk_start),
                m_chunk_end(chunk_end),
                m_total_chunk_size(total_chunk_size),
                m_buffers(buffers) {}

        input(input&& rhs) = default;

        input(const input& other) = default;

        input& operator=(input&& rhs) = default;

        input& operator=(const input& other) = default;

        std::string
        path() const {
            return m_path;
        }

        int64_t
        offset() const {
            return m_offset;
        }

        uint64_t
        host_id() const {
            return m_host_id;
        }

        uint64_t
        host_size() const {
            return m_host_size;
        }

        uint64_t
        chunk_n() const {
            return m_chunk_n;
        }

        uint64_t
        chunk_start() const {
            return m_chunk_start;
        }

        uint64_t
        chunk_end() const {
            return m_chunk_end;
        }

        uint64_t
        total_chunk_size() const {
            return m_total_chunk_size;
        }

        hermes::exposed_memory
        buffers() const {
            return m_buffers;
        }

        explicit
        input(const rpc_read_data_in_t& other) :
                m_path(other.path),
                m_offset(other.offset),
                m_host_id(other.host_id),
                m_host_size(other.host_size),
                m_chunk_n(other.chunk_n),
                m_chunk_start(other.chunk_start),
                m_chunk_end(other.chunk_end),
                m_total_chunk_size(other.total_chunk_size),
                m_buffers(other.bulk_handle) {}

        explicit
        operator rpc_read_data_in_t() {
            return {
                    m_path.c_str(),
                    m_offset,
                    m_host_id,
                    m_host_size,
                    m_chunk_n,
                    m_chunk_start,
                    m_chunk_end,
                    m_total_chunk_size,
                    hg_bulk_t(m_buffers)
            };
        }

    private:
        std::string m_path;
        int64_t m_offset;
        uint64_t m_host_id;
        uint64_t m_host_size;
        uint64_t m_chunk_n;
        uint64_t m_chunk_start;
        uint64_t m_chunk_end;
        uint64_t m_total_chunk_size;
        hermes::exposed_memory m_buffers;
    };

    class output {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        output() :
                m_err(),
                m_io_size() {}

        output(int32_t err, size_t io_size) :
                m_err(err),
                m_io_size(io_size) {}

        output(output&& rhs) = default;

        output(const output& other) = default;

        output& operator=(output&& rhs) = default;

        output& operator=(const output& other) = default;

        explicit
        output(const rpc_data_out_t& out) {
            m_err = out.err;
            m_io_size = out.io_size;
        }

        int32_t
        err() const {
            return m_err;
        }

        int64_t
        io_size() const {
            return m_io_size;
        }

    private:
        int32_t m_err;
        size_t m_io_size;
    };
};

//==============================================================================
// definitions for trunc_data
struct trunc_data {

    // forward declarations of public input/output types for this RPC
    class input;

    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = trunc_data;
    using handle_type = hermes::rpc_handle<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = rpc_trunc_in_t;
    using mercury_output_type = rpc_err_out_t;

    // RPC public identifier
    // (N.B: we reuse the same IDs assigned by Margo so that the daemon
    // understands Hermes RPCs)
    constexpr static const uint64_t public_id = 1850933248;

    // RPC internal Mercury identifier
    constexpr static const hg_id_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = gkfs::rpc::tag::truncate;

    // requires response?
    constexpr static const auto requires_response = true;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb =
            HG_GEN_PROC_NAME(rpc_trunc_in_t);

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb =
            HG_GEN_PROC_NAME(rpc_err_out_t);

    class input {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input(const std::string& path,
              uint64_t length) :
                m_path(path),
                m_length(length) {}

        input(input&& rhs) = default;

        input(const input& other) = default;

        input& operator=(input&& rhs) = default;

        input& operator=(const input& other) = default;

        std::string
        path() const {
            return m_path;
        }

        uint64_t
        length() const {
            return m_length;
        }

        explicit
        input(const rpc_trunc_in_t& other) :
                m_path(other.path),
                m_length(other.length) {}

        explicit
        operator rpc_trunc_in_t() {
            return {
                    m_path.c_str(),
                    m_length,
            };
        }

    private:
        std::string m_path;
        uint64_t m_length;
    };

    class output {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        output() :
                m_err() {}

        output(int32_t err) :
                m_err(err) {}

        output(output&& rhs) = default;

        output(const output& other) = default;

        output& operator=(output&& rhs) = default;

        output& operator=(const output& other) = default;

        explicit
        output(const rpc_err_out_t& out) {
            m_err = out.err;
        }

        int32_t
        err() const {
            return m_err;
        }

    private:
        int32_t m_err;
    };
};

//==============================================================================
// definitions for get_dirents
struct get_dirents {

    // forward declarations of public input/output types for this RPC
    class input;

    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = get_dirents;
    using handle_type = hermes::rpc_handle<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = rpc_get_dirents_in_t;
    using mercury_output_type = rpc_get_dirents_out_t;

    // RPC public identifier
    // (N.B: we reuse the same IDs assigned by Margo so that the daemon
    // understands Hermes RPCs)
    constexpr static const uint64_t public_id = 4121034752;

    // RPC internal Mercury identifier
    constexpr static const hg_id_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = gkfs::rpc::tag::get_dirents;

    // requires response?
    constexpr static const auto requires_response = true;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb =
            HG_GEN_PROC_NAME(rpc_get_dirents_in_t);

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb =
            HG_GEN_PROC_NAME(rpc_get_dirents_out_t);

    class input {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input(const std::string& path,
              const hermes::exposed_memory& buffers) :
                m_path(path),
                m_buffers(buffers) {}

        input(input&& rhs) = default;

        input(const input& other) = default;

        input& operator=(input&& rhs) = default;

        input& operator=(const input& other) = default;

        std::string
        path() const {
            return m_path;
        }

        hermes::exposed_memory
        buffers() const {
            return m_buffers;
        }

        explicit
        input(const rpc_get_dirents_in_t& other) :
                m_path(other.path),
                m_buffers(other.bulk_handle) {}

        explicit
        operator rpc_get_dirents_in_t() {
            return {
                    m_path.c_str(),
                    hg_bulk_t(m_buffers)
            };
        }

    private:
        std::string m_path;
        hermes::exposed_memory m_buffers;
    };

    class output {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        output() :
                m_err(),
                m_dirents_size() {}

        output(int32_t err, size_t dirents_size) :
                m_err(err),
                m_dirents_size(dirents_size) {}

        output(output&& rhs) = default;

        output(const output& other) = default;

        output& operator=(output&& rhs) = default;

        output& operator=(const output& other) = default;

        explicit
        output(const rpc_get_dirents_out_t& out) {
            m_err = out.err;
            m_dirents_size = out.dirents_size;
        }

        int32_t
        err() const {
            return m_err;
        }

        size_t
        dirents_size() const {
            return m_dirents_size;
        }

    private:
        int32_t m_err;
        size_t m_dirents_size;
    };
};

//==============================================================================
// definitions for chunk_stat
struct chunk_stat {

    // forward declarations of public input/output types for this RPC
    class input;

    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = chunk_stat;
    using handle_type = hermes::rpc_handle<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = rpc_chunk_stat_in_t;
    using mercury_output_type = rpc_chunk_stat_out_t;

    // RPC public identifier
    // (N.B: we reuse the same IDs assigned by Margo so that the daemon
    // understands Hermes RPCs)
    constexpr static const uint64_t public_id = 532742144;

    // RPC internal Mercury identifier
    constexpr static const hg_id_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = gkfs::rpc::tag::get_chunk_stat;

    // requires response?
    constexpr static const auto requires_response = true;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb =
            HG_GEN_PROC_NAME(rpc_chunk_stat_in_t);

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb =
            HG_GEN_PROC_NAME(rpc_chunk_stat_out_t);

    class input {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input(int32_t dummy) :
                m_dummy(dummy) {}

        input(input&& rhs) = default;

        input(const input& other) = default;

        input& operator=(input&& rhs) = default;

        input& operator=(const input& other) = default;

        int32_t
        dummy() const {
            return m_dummy;
        }

        explicit
        input(const rpc_chunk_stat_in_t& other) :
                m_dummy(other.dummy) {}

        explicit
        operator rpc_chunk_stat_in_t() {
            return {m_dummy};
        }

    private:
        int32_t m_dummy;
    };

    class output {

        template<typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        output() :
                m_err(),
                m_chunk_size(),
                m_chunk_total(),
                m_chunk_free() {}

        output(int32_t err, uint64_t chunk_size, uint64_t chunk_total, uint64_t chunk_free) :
                m_err(err),
                m_chunk_size(chunk_size),
                m_chunk_total(chunk_total),
                m_chunk_free(chunk_free) {}

        output(output&& rhs) = default;

        output(const output& other) = default;

        output& operator=(output&& rhs) = default;

        output& operator=(const output& other) = default;

        explicit
        output(const rpc_chunk_stat_out_t& out) {
            m_err = out.err;
            m_chunk_size = out.chunk_size;
            m_chunk_total = out.chunk_total;
            m_chunk_free = out.chunk_free;
        }

        int32_t
        err() const {
            return m_err;
        }

        uint64_t
        chunk_size() const {
            return m_chunk_size;
        }

        uint64_t
        chunk_total() const {
            return m_chunk_total;
        }

        uint64_t
        chunk_free() const {
            return m_chunk_free;
        }

    private:
        int32_t m_err;
        uint64_t m_chunk_size;
        uint64_t m_chunk_total;
        uint64_t m_chunk_free;
    };
};

} // namespace rpc
} // namespace gkfs


#endif // GKFS_RPCS_TYPES_HPP
