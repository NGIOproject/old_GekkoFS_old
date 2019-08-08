/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#ifndef IFS_METADENTRY_HPP
#define IFS_METADENTRY_HPP

#include <daemon/main.hpp>
#include <global/metadata.hpp>

int create_node(const std::string& path, const uid_t uid, const gid_t gid, mode_t mode);

void create_metadentry(const std::string& path, Metadata& md);

std::string get_metadentry_str(const std::string& path);

Metadata get_metadentry(const std::string& path);

void remove_node(const std::string& path);

size_t get_metadentry_size(const std::string& path);

void update_metadentry_size(const std::string& path, size_t io_size, off_t offset, bool append);

void update_metadentry(const std::string& path, Metadata& md);

std::vector<std::pair<std::string, bool>> get_dirents(const std::string& dir);

#endif //IFS_METADENTRY_HPP
