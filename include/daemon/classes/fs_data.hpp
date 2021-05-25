/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#ifndef LFS_FS_DATA_H
#define LFS_FS_DATA_H

#include <daemon/daemon.hpp>

#include <unordered_map>
#include <map>
#include <functional> //std::hash

/* Forward declarations */
namespace gkfs {
namespace metadata {
class MetadataDB;
}

namespace data {
class ChunkStorage;
}

namespace daemon {

class FsData {

private:
    FsData() {}

    // logger
    std::shared_ptr<spdlog::logger> spdlogger_;

    // paths
    std::string rootdir_;
    std::string mountdir_;
    std::string metadir_;

    // RPC management
    std::string rpc_protocol_;
    std::string bind_addr_;
    std::string hosts_file_;
    bool use_auto_sm_;

    // Database
    std::shared_ptr<gkfs::metadata::MetadataDB> mdb_;
    // Storage backend
    std::shared_ptr<gkfs::data::ChunkStorage> storage_;

    // configurable metadata
    bool atime_state_;
    bool mtime_state_;
    bool ctime_state_;
    bool link_cnt_state_;
    bool blocks_state_;

public:
    static FsData* getInstance() {
        static FsData instance;
        return &instance;
    }

    FsData(FsData const&) = delete;

    void operator=(FsData const&) = delete;

    // getter/setter

    const std::shared_ptr<spdlog::logger>& spdlogger() const;

    void spdlogger(const std::shared_ptr<spdlog::logger>& spdlogger_);

    const std::string& rootdir() const;

    void rootdir(const std::string& rootdir_);

    const std::string& mountdir() const;

    void mountdir(const std::string& mountdir_);

    const std::string& metadir() const;

    void metadir(const std::string& metadir_);

    const std::shared_ptr<gkfs::metadata::MetadataDB>& mdb() const;

    void mdb(const std::shared_ptr<gkfs::metadata::MetadataDB>& mdb);

    void close_mdb();

    const std::shared_ptr<gkfs::data::ChunkStorage>& storage() const;

    void storage(const std::shared_ptr<gkfs::data::ChunkStorage>& storage);

    const std::string& rpc_protocol() const;

    void rpc_protocol(const std::string& rpc_protocol);

    const std::string& bind_addr() const;

    void bind_addr(const std::string& addr);

    const std::string& hosts_file() const;

    bool use_auto_sm() const;

    void use_auto_sm(bool use_auto_sm);

    void hosts_file(const std::string& lookup_file);

    bool atime_state() const;

    void atime_state(bool atime_state);

    bool mtime_state() const;

    void mtime_state(bool mtime_state);

    bool ctime_state() const;

    void ctime_state(bool ctime_state);

    bool link_cnt_state() const;

    void link_cnt_state(bool link_cnt_state);

    bool blocks_state() const;

    void blocks_state(bool blocks_state);

};

} // namespace daemon
} // namespace gkfs

#endif //LFS_FS_DATA_H
