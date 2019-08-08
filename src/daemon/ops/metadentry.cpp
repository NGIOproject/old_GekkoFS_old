/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#include <daemon/ops/metadentry.hpp>
#include <daemon/backend/metadata/db.hpp>
#include <daemon/backend/data/chunk_storage.hpp>

using namespace std;

/**
 * Creates metadata (if required) and dentry at the same time
 * @param path
 * @param mode
 */
void create_metadentry(const std::string& path, Metadata& md) {

    // update metadata object based on what metadata is needed
    if (ADAFS_DATA->atime_state() || ADAFS_DATA->mtime_state() || ADAFS_DATA->ctime_state()) {
        std::time_t time;
        std::time(&time);
        auto time_s = fmt::format_int(time).str();
        if (ADAFS_DATA->atime_state())
            md.atime(time);
        if (ADAFS_DATA->mtime_state())
            md.mtime(time);
        if (ADAFS_DATA->ctime_state())
            md.ctime(time);
    }
    ADAFS_DATA->mdb()->put(path, md.serialize());
}

std::string get_metadentry_str(const std::string& path) {
        return ADAFS_DATA->mdb()->get(path);
}

/**
 * Returns the metadata of an object at a specific path. The metadata can be of dummy values if configured
 * @param path
 * @param attr
 * @return
 */
Metadata get_metadentry(const std::string& path) {
    return Metadata(get_metadentry_str(path));
}

/**
 * Remove metadentry if exists and try to remove all chunks for path
 * @param path
 * @return
 */
void remove_node(const string& path) {
    ADAFS_DATA->mdb()->remove(path); // remove metadentry
    ADAFS_DATA->storage()->destroy_chunk_space(path); // destroys all chunks for the path on this node
}

/**
 * Gets the size of a metadentry
 * @param path
 * @param ret_size (return val)
 * @return err
 */
size_t get_metadentry_size(const string& path) {
    return get_metadentry(path).size();
}

/**
 * Updates a metadentry's size atomically and returns the corresponding size after update
 * @param path
 * @param io_size
 * @return the updated size
 */
void update_metadentry_size(const string& path, size_t io_size, off64_t offset, bool append) {
    ADAFS_DATA->mdb()->increase_size(path, io_size + offset, append);
}

void update_metadentry(const string& path, Metadata& md) {
    ADAFS_DATA->mdb()->update(path, path, md.serialize());
}

std::vector<std::pair<std::string, bool>> get_dirents(const std::string& dir){
    return ADAFS_DATA->mdb()->get_dirents(dir);
}