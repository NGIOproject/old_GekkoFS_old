/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <global/metadata.hpp>
#include <config.hpp>

#include <fmt/format.h>

extern "C" {
#include <sys/stat.h>
#include <unistd.h>
}

#include <ctime>
#include <cassert>

namespace gkfs {
namespace metadata {

static const char MSP = '|'; // metadata separator

Metadata::Metadata(const mode_t mode) :
        atime_(),
        mtime_(),
        ctime_(),
        mode_(mode),
        link_count_(0),
        size_(0),
        blocks_(0) {
    assert(S_ISDIR(mode_) || S_ISREG(mode_));
}

#ifdef HAS_SYMLINKS

Metadata::Metadata(const mode_t mode, const std::string& target_path) :
        atime_(),
        mtime_(),
        ctime_(),
        mode_(mode),
        link_count_(0),
        size_(0),
        blocks_(0),
        target_path_(target_path) {
    assert(S_ISLNK(mode_) || S_ISDIR(mode_) || S_ISREG(mode_));
    // target_path should be there only if this is a link
    assert(target_path_.empty() || S_ISLNK(mode_));
    // target_path should be absolute
    assert(target_path_.empty() || target_path_[0] == '/');
}

#endif

Metadata::Metadata(const std::string& binary_str) {
    size_t read = 0;

    auto ptr = binary_str.data();
    mode_ = static_cast<unsigned int>(std::stoul(ptr, &read));
    // we read something
    assert(read > 0);
    ptr += read;

    // last parsed char is the separator char
    assert(*ptr == MSP);
    // yet we have some character to parse

    size_ = std::stol(++ptr, &read);
    assert(read > 0);
    ptr += read;

    // The order is important. don't change.
    if (gkfs::config::metadata::use_atime) {
        assert(*ptr == MSP);
        atime_ = static_cast<time_t>(std::stol(++ptr, &read));
        assert(read > 0);
        ptr += read;
    }
    if (gkfs::config::metadata::use_mtime) {
        assert(*ptr == MSP);
        mtime_ = static_cast<time_t>(std::stol(++ptr, &read));
        assert(read > 0);
        ptr += read;
    }
    if (gkfs::config::metadata::use_ctime) {
        assert(*ptr == MSP);
        ctime_ = static_cast<time_t>(std::stol(++ptr, &read));
        assert(read > 0);
        ptr += read;
    }
    if (gkfs::config::metadata::use_link_cnt) {
        assert(*ptr == MSP);
        link_count_ = static_cast<nlink_t>(std::stoul(++ptr, &read));
        assert(read > 0);
        ptr += read;
    }
    if (gkfs::config::metadata::use_blocks) { // last one will not encounter a delimiter anymore
        assert(*ptr == MSP);
        blocks_ = static_cast<blkcnt_t>(std::stoul(++ptr, &read));
        assert(read > 0);
        ptr += read;
    }

#ifdef HAS_SYMLINKS
    // Read target_path
    assert(*ptr == MSP);
    target_path_ = ++ptr;
    // target_path should be there only if this is a link
    assert(target_path_.empty() || S_ISLNK(mode_));
    ptr += target_path_.size();
#endif

    // we consumed all the binary string
    assert(*ptr == '\0');
}

std::string Metadata::serialize() const {
    std::string s;
    // The order is important. don't change.
    s += fmt::format_int(mode_).c_str(); // add mandatory mode
    s += MSP;
    s += fmt::format_int(size_).c_str(); // add mandatory size
    if (gkfs::config::metadata::use_atime) {
        s += MSP;
        s += fmt::format_int(atime_).c_str();
    }
    if (gkfs::config::metadata::use_mtime) {
        s += MSP;
        s += fmt::format_int(mtime_).c_str();
    }
    if (gkfs::config::metadata::use_ctime) {
        s += MSP;
        s += fmt::format_int(ctime_).c_str();
    }
    if (gkfs::config::metadata::use_link_cnt) {
        s += MSP;
        s += fmt::format_int(link_count_).c_str();
    }
    if (gkfs::config::metadata::use_blocks) {
        s += MSP;
        s += fmt::format_int(blocks_).c_str();
    }

#ifdef HAS_SYMLINKS
    s += MSP;
    s += target_path_;
#endif

    return s;
}

void Metadata::init_ACM_time() {
    std::time_t time;
    std::time(&time);
    atime_ = time;
    mtime_ = time;
    ctime_ = time;
}

void Metadata::update_ACM_time(bool a, bool c, bool m) {
    std::time_t time;
    std::time(&time);
    if (a)
        atime_ = time;
    if (c)
        ctime_ = time;
    if (m)
        mtime_ = time;
}

//-------------------------------------------- GETTER/SETTER

time_t Metadata::atime() const {
    return atime_;
}

void Metadata::atime(time_t atime) {
    Metadata::atime_ = atime;
}

time_t Metadata::mtime() const {
    return mtime_;
}

void Metadata::mtime(time_t mtime) {
    Metadata::mtime_ = mtime;
}

time_t Metadata::ctime() const {
    return ctime_;
}

void Metadata::ctime(time_t ctime) {
    Metadata::ctime_ = ctime;
}

mode_t Metadata::mode() const {
    return mode_;
}

void Metadata::mode(mode_t mode) {
    Metadata::mode_ = mode;
}

nlink_t Metadata::link_count() const {
    return link_count_;
}

void Metadata::link_count(nlink_t link_count) {
    Metadata::link_count_ = link_count;
}

size_t Metadata::size() const {
    return size_;
}

void Metadata::size(size_t size) {
    Metadata::size_ = size;
}

blkcnt_t Metadata::blocks() const {
    return blocks_;
}

void Metadata::blocks(blkcnt_t blocks) {
    Metadata::blocks_ = blocks;
}

#ifdef HAS_SYMLINKS

std::string Metadata::target_path() const {
    assert(!target_path_.empty());
    return target_path_;
}

void Metadata::target_path(const std::string& target_path) {
    // target_path should be there only if this is a link
    assert(target_path.empty() || S_ISLNK(mode_));
    // target_path should be absolute
    assert(target_path.empty() || target_path[0] == '/');
    target_path_ = target_path;
}

bool Metadata::is_link() const {
    return S_ISLNK(mode_);
}

#endif

} // namespace metadata
} // namespace gkfs
