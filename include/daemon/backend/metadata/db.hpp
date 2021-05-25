/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GEKKOFS_METADATA_DB_HPP
#define GEKKOFS_METADATA_DB_HPP

#include <memory>
#include <rocksdb/db.h>
#include <daemon/backend/exceptions.hpp>

namespace rdb = rocksdb;

namespace gkfs {
namespace metadata {

class MetadataDB {
private:
    std::unique_ptr<rdb::DB> db;
    rdb::Options options;
    rdb::WriteOptions write_opts;
    std::string path;

    static void optimize_rocksdb_options(rdb::Options& options);

public:
    static inline void throw_rdb_status_excpt(const rdb::Status& s);

    MetadataDB(const std::string& path);

    std::string get(const std::string& key) const;

    void put(const std::string& key, const std::string& val);

    void remove(const std::string& key);

    bool exists(const std::string& key);

    void update(const std::string& old_key, const std::string& new_key, const std::string& val);

    void increase_size(const std::string& key, size_t size, bool append);

    void decrease_size(const std::string& key, size_t size);

    std::vector<std::pair<std::string, bool>> get_dirents(const std::string& dir) const;

    void iterate_all();
};

} // namespace metadata
} // namespace gkfs

#endif //GEKKOFS_METADATA_DB_HPP
