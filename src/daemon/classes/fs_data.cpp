/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/
#include <daemon/classes/fs_data.hpp>
#include <daemon/backend/metadata/db.hpp>

#include <spdlog/spdlog.h>

namespace gkfs {
namespace daemon {

// getter/setter

const std::shared_ptr<spdlog::logger>& FsData::spdlogger() const {
    return spdlogger_;
}

void FsData::spdlogger(const std::shared_ptr<spdlog::logger>& spdlogger) {
    FsData::spdlogger_ = spdlogger;
}

const std::shared_ptr<gkfs::metadata::MetadataDB>& FsData::mdb() const {
    return mdb_;
}

void FsData::mdb(const std::shared_ptr<gkfs::metadata::MetadataDB>& mdb) {
    mdb_ = mdb;
}

void FsData::close_mdb() {
    mdb_.reset();
}

const std::shared_ptr<gkfs::data::ChunkStorage>& FsData::storage() const {
    return storage_;
}

void FsData::storage(const std::shared_ptr<gkfs::data::ChunkStorage>& storage) {
    storage_ = storage;
}

const std::string& FsData::rootdir() const {
    return rootdir_;
}

void FsData::rootdir(const std::string& rootdir) {
    FsData::rootdir_ = rootdir;
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

const std::string& FsData::rpc_protocol() const {
    return rpc_protocol_;
}

void FsData::rpc_protocol(const std::string& rpc_protocol) {
    rpc_protocol_ = rpc_protocol;
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

bool FsData::use_auto_sm() const {
    return use_auto_sm_;
}

void FsData::use_auto_sm(bool use_auto_sm) {
    use_auto_sm_ = use_auto_sm;
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

} // namespace daemon
} // namespace gkfs






