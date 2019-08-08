/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <daemon/backend/metadata/db.hpp>
#include <daemon/backend/metadata/merge.hpp>
#include <daemon/backend/exceptions.hpp>

#include <global/metadata.hpp>
#include <global/path_util.hpp>

#include <sys/stat.h>


MetadataDB::MetadataDB(const std::string& path): path(path) {
    // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    // create the DB if it's not already present
    options.create_if_missing = true;
    options.merge_operator.reset(new MetadataMergeOperator);
    MetadataDB::optimize_rocksdb_options(options);
    write_opts.disableWAL = !(KV_WOL);
    rdb::DB * rdb_ptr;
    auto s = rocksdb::DB::Open(options, path, &rdb_ptr);
    if (!s.ok()) {
        throw std::runtime_error("Failed to open RocksDB: " + s.ToString());
    }
    this->db.reset(rdb_ptr);
}

void MetadataDB::throw_rdb_status_excpt(const rdb::Status& s){
    assert(!s.ok());

    if(s.IsNotFound()){
        throw NotFoundException(s.ToString());
    } else {
        throw DBException(s.ToString());
    }
}

std::string MetadataDB::get(const std::string& key) const {
    std::string val;
    auto s = db->Get(rdb::ReadOptions(), key, &val);
    if(!s.ok()){
        MetadataDB::throw_rdb_status_excpt(s);
    }
    return val;
}

void MetadataDB::put(const std::string& key, const std::string& val) {
    assert(is_absolute_path(key));
    assert(key == "/" || !has_trailing_slash(key));

    auto cop = CreateOperand(val);
    auto s = db->Merge(write_opts, key, cop.serialize());
    if(!s.ok()){
        MetadataDB::throw_rdb_status_excpt(s);
    }
}

void MetadataDB::remove(const std::string& key) {
    auto s = db->Delete(write_opts, key);
    if(!s.ok()){
        MetadataDB::throw_rdb_status_excpt(s);
    }
}

bool MetadataDB::exists(const std::string& key) {
    std::string val;
    auto s = db->Get(rdb::ReadOptions(), key, &val);
    if(!s.ok()){
        if(s.IsNotFound()){
            return false;
        } else {
            MetadataDB::throw_rdb_status_excpt(s);
        }
    }
    return true;
}

/**
 * Updates a metadentry atomically and also allows to change keys
 * @param old_key
 * @param new_key
 * @param val
 * @return
 */
void MetadataDB::update(const std::string& old_key, const std::string& new_key, const std::string& val) {
    //TODO use rdb::Put() method
    rdb::WriteBatch batch;
    batch.Delete(old_key);
    batch.Put(new_key, val);
    auto s = db->Write(write_opts, &batch);
    if(!s.ok()){
        MetadataDB::throw_rdb_status_excpt(s);
    }
}

void MetadataDB::increase_size(const std::string& key, size_t size, bool append){
    auto uop = IncreaseSizeOperand(size, append);
    auto s = db->Merge(write_opts, key, uop.serialize());
    if(!s.ok()){
        MetadataDB::throw_rdb_status_excpt(s);
    }
}

void MetadataDB::decrease_size(const std::string& key, size_t size) {
    auto uop = DecreaseSizeOperand(size);
    auto s = db->Merge(write_opts, key, uop.serialize());
    if(!s.ok()){
        MetadataDB::throw_rdb_status_excpt(s);
    }
}

/**
 * Return all the first-level entries of the directory @dir
 *
 * @return vector of pair <std::string name, bool is_dir>,
 *         where name is the name of the entries and is_dir
 *         is true in the case the entry is a directory.
 */
std::vector<std::pair<std::string, bool>> MetadataDB::get_dirents(const std::string& dir) const {
    auto root_path = dir;
    assert(is_absolute_path(root_path));
    //add trailing slash if missing
    if(!has_trailing_slash(root_path) && root_path.size() != 1) {
        //add trailing slash only if missing and is not the root_folder "/"
        root_path.push_back('/');
    }

    rocksdb::ReadOptions ropts;
    auto it = db->NewIterator(ropts);

    std::vector<std::pair<std::string, bool>> entries;

    for(it->Seek(root_path);
            it->Valid() &&
            it->key().starts_with(root_path);
        it->Next()){

        if(it->key().size() == root_path.size()) {
            //we skip this path cause it is exactly the root_path
            continue;
        }

        /***** Get File name *****/
        auto name = it->key().ToString();
        if(name.find_first_of('/', root_path.size()) != std::string::npos){
            //skip stuff deeper then one level depth
            continue;
        }
        // remove prefix
        name = name.substr(root_path.size());

        //relative path of directory entries must not be empty
        assert(name.size() > 0);

        Metadata md(it->value().ToString());
        auto is_dir = S_ISDIR(md.mode());

        entries.push_back(std::make_pair(std::move(name), std::move(is_dir)));
    }
    assert(it->status().ok());
    return entries;
}

void MetadataDB::iterate_all() {
    std::string key;
    std::string val;
    // Do RangeScan on parent inode
    auto iter = db->NewIterator(rdb::ReadOptions());
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        key = iter->key().ToString();
        val = iter->value().ToString();
    }
}

void MetadataDB::optimize_rocksdb_options(rdb::Options& options) {
    options.max_successive_merges = 128;

#if defined(KV_WRITE_BUFFER)
    // write_buffer_size is multiplied by the write_buffer_number to get the amount of data hold in memory.
    // at min_write_buffer_number_to_merge rocksdb starts to flush entries out to disk
    options.write_buffer_size = KV_WRITE_BUFFER << 20;
    // XXX experimental values. We only want one buffer, which is held in memory
    options.max_write_buffer_number = 1;
    options.min_write_buffer_number_to_merge = 1;
#endif
}
