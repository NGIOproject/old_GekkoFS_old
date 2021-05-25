/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <global/rpc/distributor.hpp>

using namespace std;

namespace gkfs {
namespace rpc {

SimpleHashDistributor::
SimpleHashDistributor(host_t localhost, unsigned int hosts_size) :
        localhost_(localhost),
        hosts_size_(hosts_size),
        all_hosts_(hosts_size) {
    ::iota(all_hosts_.begin(), all_hosts_.end(), 0);
}

host_t SimpleHashDistributor::
localhost() const {
    return localhost_;
}

host_t SimpleHashDistributor::
locate_data(const string& path, const chunkid_t& chnk_id) const {
    return str_hash(path + ::to_string(chnk_id)) % hosts_size_;
}

host_t SimpleHashDistributor::
locate_file_metadata(const string& path) const {
    return str_hash(path) % hosts_size_;
}

::vector<host_t> SimpleHashDistributor::
locate_directory_metadata(const string& path) const {
    return all_hosts_;
}

LocalOnlyDistributor::LocalOnlyDistributor(host_t localhost) : localhost_(localhost) {}

host_t LocalOnlyDistributor::
localhost() const {
    return localhost_;
}

host_t LocalOnlyDistributor::
locate_data(const string& path, const chunkid_t& chnk_id) const {
    return localhost_;
}

host_t LocalOnlyDistributor::
locate_file_metadata(const string& path) const {
    return localhost_;
}

::vector<host_t> LocalOnlyDistributor::
locate_directory_metadata(const string& path) const {
    return {localhost_};
}

ForwarderDistributor::
ForwarderDistributor(host_t fwhost, unsigned int hosts_size) :
        fwd_host_(fwhost),
        hosts_size_(hosts_size),
        all_hosts_(hosts_size) {
    ::iota(all_hosts_.begin(), all_hosts_.end(), 0);
}

host_t ForwarderDistributor::
localhost() const {
    return fwd_host_;
}

host_t ForwarderDistributor::
locate_data(const std::string& path, const chunkid_t& chnk_id) const {
    return fwd_host_;
}

host_t ForwarderDistributor::
locate_file_metadata(const std::string& path) const {
    return str_hash(path) % hosts_size_;
}

std::vector<host_t> ForwarderDistributor::
locate_directory_metadata(const std::string& path) const {
    return all_hosts_;
}
} // namespace rpc
} // namespace gkfs
