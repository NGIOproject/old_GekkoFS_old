/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <global/rpc/distributor.hpp>

SimpleHashDistributor::
SimpleHashDistributor(Host localhost, unsigned int hosts_size) :
    localhost_(localhost),
    hosts_size_(hosts_size),
    all_hosts_(hosts_size)
{
    std::iota(all_hosts_.begin(), all_hosts_.end(), 0);
}

Host SimpleHashDistributor::
localhost() const {
    return localhost_;
}

Host SimpleHashDistributor::
locate_data(const std::string& path, const ChunkID& chnk_id) const {
    return str_hash(path + std::to_string(chnk_id)) % hosts_size_;
}

Host SimpleHashDistributor::
locate_file_metadata(const std::string& path) const {
    return str_hash(path) % hosts_size_;
}


std::vector<Host> SimpleHashDistributor::
locate_directory_metadata(const std::string& path) const {
    return all_hosts_;
}


LocalOnlyDistributor::LocalOnlyDistributor(Host localhost) : localhost_(localhost)
{}

Host LocalOnlyDistributor::
localhost() const {
    return localhost_;
}

Host LocalOnlyDistributor::
locate_data(const std::string& path, const ChunkID& chnk_id) const {
    return localhost_;
}

Host LocalOnlyDistributor::
locate_file_metadata(const std::string& path) const {
    return localhost_;
}

std::vector<Host> LocalOnlyDistributor::
locate_directory_metadata(const std::string& path) const {
    return {localhost_};
}
