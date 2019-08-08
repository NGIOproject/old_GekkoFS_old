/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#ifndef LFS_FS_DATA_H
#define LFS_FS_DATA_H

#include <daemon/main.hpp>

/* Forward declarations */
class MetadataDB;
class ChunkStorage;

#include <unordered_map>
#include <map>
#include <functional> //std::hash

class FsData {

private:
    FsData() {}

    // Caching
    std::unordered_map<std::string, std::string> hashmap_;
    std::hash<std::string> hashf_;

    // Later the blocksize will likely be coupled to the chunks to allow individually big chunk sizes.
    blksize_t blocksize_;

    //logger
    std::shared_ptr<spdlog::logger> spdlogger_;

    // paths
    std::string rootdir_;
    std::string mountdir_;
    std::string metadir_;

    std::string bind_addr_;
    std::string hosts_file_;

    // Database
    std::shared_ptr<MetadataDB> mdb_;
    // Storage backend
    std::shared_ptr<ChunkStorage> storage_;

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

    const std::unordered_map<std::string, std::string>& hashmap() const;

    void hashmap(const std::unordered_map<std::string, std::string>& hashmap_);

    const std::hash<std::string>& hashf() const;

    void hashf(const std::hash<std::string>& hashf_);

    blksize_t blocksize() const;

    void blocksize(blksize_t blocksize_);

    const std::shared_ptr<spdlog::logger>& spdlogger() const;

    void spdlogger(const std::shared_ptr<spdlog::logger>& spdlogger_);

    const std::string& rootdir() const;

    void rootdir(const std::string& rootdir_);

    const std::string& mountdir() const;

    void mountdir(const std::string& mountdir_);

    const std::string& metadir() const;

    void metadir(const std::string& metadir_);

    const std::shared_ptr<MetadataDB>& mdb() const;

    void mdb(const std::shared_ptr<MetadataDB>& mdb);

    void close_mdb();

    const std::shared_ptr<ChunkStorage>& storage() const;

    void storage(const std::shared_ptr<ChunkStorage>& storage);

    const std::string& bind_addr() const;

    void bind_addr(const std::string& addr);

    const std::string& hosts_file() const;

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


#endif //LFS_FS_DATA_H
