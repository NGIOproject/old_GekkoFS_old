/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#ifndef GEKKOFS_OPEN_DIR_HPP
#define GEKKOFS_OPEN_DIR_HPP

#include <string>
#include <vector>

#include <client/open_file_map.hpp>

namespace gkfs {
namespace filemap {

class DirEntry {
private:
    std::string name_;
    FileType type_;
public:
    DirEntry(const std::string& name, FileType type);

    const std::string& name();

    FileType type();
};

class OpenDir : public OpenFile {
private:
    std::vector<DirEntry> entries;


public:
    explicit OpenDir(const std::string& path);

    void add(const std::string& name, const FileType& type);

    const DirEntry& getdent(unsigned int pos);

    size_t size();
};

} // namespace filemap
} // namespace gkfs

#endif //GEKKOFS_OPEN_DIR_HPP
