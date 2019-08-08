/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#ifndef IFS_OPEN_DIR_HPP
#define IFS_OPEN_DIR_HPP

#include <string>
#include <vector>

#include <dirent.h>

#include <client/open_file_map.hpp>


class DirEntry {
    private:
        std::string name_;
        FileType type_;
    public:
        DirEntry(const std::string& name, const FileType type);
        const std::string& name();
        FileType type();
};

class OpenDir: public OpenFile {
    private:
        std::vector<DirEntry> entries;


    public:
        OpenDir(const std::string& path);
        void add(const std::string& name, const FileType& type);
        const DirEntry& getdent(unsigned int pos);
        size_t size();
};


#endif //IFS_OPEN_DIR_HPP
