/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#ifndef GEKKOFS_METADENTRY_HPP
#define GEKKOFS_METADENTRY_HPP

#include <daemon/daemon.hpp>
#include <global/metadata.hpp>

namespace gkfs {
namespace metadata {

Metadata get(const std::string& path);

std::string get_str(const std::string& path);

size_t get_size(const std::string& path);

std::vector<std::pair<std::string, bool>> get_dirents(const std::string& dir);

void create(const std::string& path, Metadata& md);

void update(const std::string& path, Metadata& md);

void update_size(const std::string& path, size_t io_size, off_t offset, bool append);

void remove(const std::string& path);

} // namespace metadata
} // namespace gkfs

#endif //GEKKOFS_METADENTRY_HPP
