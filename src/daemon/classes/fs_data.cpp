/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <spdlog/spdlog.h>

#include <daemon/classes/fs_data.hpp>
#include <daemon/backend/metadata/db.hpp>


// getter/setter

const std::unordered_map<std::string, std::string>& FsData::hashmap() const {
    return hashmap_;
}

void FsData::hashmap(const std::unordered_map<std::string, std::string>& hashmap_) {
    FsData::hashmap_ = hashmap_;
}

const std::hash<std::string>& FsData::hashf() const {
    return hashf_;
}

void FsData::hashf(const std::hash<std::string>& hashf_) {
    FsData::hashf_ = hashf_;
}

blksize_t FsData::blocksize() const {
    return blocksize_;
}

void FsData::blocksize(blksize_t blocksize_) {
    FsData::blocksize_ = blocksize_;
}

const std::shared_ptr<spdlog::logger>& FsData::spdlogger() const {
    return spdlogger_;
}

void FsData::spdlogger(const std::shared_ptr<spdlog::logger>& spdlogger_) {
    FsData::spdlogger_ = spdlogger_;
}

const std::shared_ptr<MetadataDB>& FsData::mdb() const {
    return mdb_;
}

void FsData::mdb(const std::shared_ptr<MetadataDB>& mdb) {
    mdb_ = mdb;
}

void FsData::close_mdb() {
    mdb_.reset();
}

const std::shared_ptr<ChunkStorage>& FsData::storage() const {
    return storage_;
}

void FsData::storage(const std::shared_ptr<ChunkStorage>& storage) {
    storage_ = storage;
}

const std::string& FsData::rootdir() const {
    return rootdir_;
}

void FsData::rootdir(const std::string& rootdir_) {
    FsData::rootdir_ = rootdir_;
}

const std::string& FsData::mountdir() const {
    return mountdir_;
}

void FsData::mountdir(const std::string& mountdir) {
    FsData::mountdir_ = mountdir;
}

const std::string& FsData::metadir() const {
    return metadir_;
}

void FsData::metadir(const std::string& metadir) {
    FsData::metadir_ = metadir;
}

const std::string& FsData::bind_addr() const {
    return bind_addr_;
}

void FsData::bind_addr(const std::string& addr) {
    bind_addr_ = addr;
}

const std::string& FsData::hosts_file() const {
    return hosts_file_;
}

void FsData::hosts_file(const std::string& lookup_file) {
    hosts_file_ = lookup_file;
}

bool FsData::atime_state() const {
    return atime_state_;
}

void FsData::atime_state(bool atime_state) {
    FsData::atime_state_ = atime_state;
}

bool FsData::mtime_state() const {
    return mtime_state_;
}

void FsData::mtime_state(bool mtime_state) {
    FsData::mtime_state_ = mtime_state;
}

bool FsData::ctime_state() const {
    return ctime_state_;
}

void FsData::ctime_state(bool ctime_state) {
    FsData::ctime_state_ = ctime_state;
}

bool FsData::link_cnt_state() const {
    return link_cnt_state_;
}

void FsData::link_cnt_state(bool link_cnt_state) {
    FsData::link_cnt_state_ = link_cnt_state;
}

bool FsData::blocks_state() const {
    return blocks_state_;
}

void FsData::blocks_state(bool blocks_state) {
    FsData::blocks_state_ = blocks_state;
}








