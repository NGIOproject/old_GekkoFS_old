/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <client/open_dir.hpp>
#include <stdexcept>
#include <cstring>
#include <cassert>


DirEntry::DirEntry(const std::string& name, const FileType type):
    name_(name), type_(type) {
}

const std::string& DirEntry::name() {
    return name_;
}

FileType DirEntry::type() {
    return type_;
}


OpenDir::OpenDir(const std::string& path) :
    OpenFile(path, 0, FileType::directory) {
}


void OpenDir::add(const std::string& name, const FileType& type) {
    entries.push_back(DirEntry(name, type));
}

const DirEntry& OpenDir::getdent(unsigned int pos) {
    return entries.at(pos);
}

size_t OpenDir::size() {
    return entries.size();
}
