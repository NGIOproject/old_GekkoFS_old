/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GEKKOFS_RPC_DISTRIBUTOR_HPP
#define GEKKOFS_RPC_DISTRIBUTOR_HPP

#include <vector>
#include <string>
#include <numeric>

namespace gkfs {
namespace rpc {

using chunkid_t = unsigned int;
using host_t = unsigned int;

class Distributor {
public:
    virtual host_t localhost() const = 0;

    virtual host_t locate_data(const std::string& path, const chunkid_t& chnk_id) const = 0;

    virtual host_t locate_file_metadata(const std::string& path) const = 0;

    virtual std::vector<host_t> locate_directory_metadata(const std::string& path) const = 0;
};


class SimpleHashDistributor : public Distributor {
private:
    host_t localhost_;
    unsigned int hosts_size_;
    std::vector<host_t> all_hosts_;
    std::hash<std::string> str_hash;
public:
    SimpleHashDistributor(host_t localhost, unsigned int hosts_size);

    host_t localhost() const override;

    host_t locate_data(const std::string& path, const chunkid_t& chnk_id) const override;

    host_t locate_file_metadata(const std::string& path) const override;

    std::vector<host_t> locate_directory_metadata(const std::string& path) const override;
};

class LocalOnlyDistributor : public Distributor {
private:
    host_t localhost_;
public:
    explicit LocalOnlyDistributor(host_t localhost);

    host_t localhost() const override;

    host_t locate_data(const std::string& path, const chunkid_t& chnk_id) const override;

    host_t locate_file_metadata(const std::string& path) const override;

    std::vector<host_t> locate_directory_metadata(const std::string& path) const override;
};

class ForwarderDistributor : public Distributor {
private:
    host_t fwd_host_;
    unsigned int hosts_size_;
    std::vector<host_t> all_hosts_;
    std::hash<std::string> str_hash;
public:
    ForwarderDistributor(host_t fwhost, unsigned int hosts_size);

    host_t localhost() const override final;

    host_t locate_data(const std::string& path, const chunkid_t& chnk_id) const override final;

    host_t locate_file_metadata(const std::string& path) const override;

    std::vector<host_t> locate_directory_metadata(const std::string& path) const override;
};

} // namespace rpc
} // namespace gkfs

#endif //GEKKOFS_RPC_LOCATOR_HPP
