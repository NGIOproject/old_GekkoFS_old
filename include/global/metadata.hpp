/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef FS_METADATA_H
#define FS_METADATA_H
#pragma once


#include "global/configure.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <string>


constexpr mode_t LINK_MODE = ((S_IRWXU | S_IRWXG | S_IRWXO) | S_IFLNK);


class Metadata {
private:
    time_t atime_;         // access time. gets updated on file access unless mounted with noatime
    time_t mtime_;         // modify time. gets updated when file content is modified.
    time_t ctime_;         // change time. gets updated when the file attributes are changed AND when file content is modified.
    mode_t mode_;
    nlink_t link_count_;   // number of names for this inode (hardlinks)
    size_t size_;          // size_ in bytes, might be computed instead of stored
    blkcnt_t blocks_;      // allocated file system blocks_
#ifdef HAS_SYMLINKS
    std::string target_path_;  // For links this is the path of the target file
#endif


public:
    Metadata();
    explicit Metadata(mode_t mode);
#ifdef HAS_SYMLINKS
    Metadata(mode_t mode, const std::string& target_path);
#endif
    // Construct from a binary representation of the object
    explicit Metadata(const std::string& binary_str);

    std::string serialize() const;

    void init_ACM_time();
    void update_ACM_time(bool a, bool c, bool m);

    //Getter and Setter
    time_t atime() const;
    void atime(time_t atime_);
    time_t mtime() const;
    void mtime(time_t mtime_);
    time_t ctime() const;
    void ctime(time_t ctime_);
    mode_t mode() const;
    void mode(mode_t mode_);
    nlink_t link_count() const;
    void link_count(nlink_t link_count_);
    size_t size() const;
    void size(size_t size_);
    blkcnt_t blocks() const;
    void blocks(blkcnt_t blocks_);
#ifdef HAS_SYMLINKS
    std::string target_path() const;
    void target_path(const std::string& target_path);
    bool is_link() const;
#endif
};


#endif //FS_METADATA_H
