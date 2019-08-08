/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef IFS_RPC_DISTRIBUTOR_HPP
#define IFS_RPC_DISTRIBUTOR_HPP

#include <vector>
#include <string>
#include <numeric>


using ChunkID = unsigned int;
using Host = unsigned int;

class Distributor {
    public:
        virtual Host localhost() const = 0;
        virtual Host locate_data(const std::string& path, const ChunkID& chnk_id) const = 0;
        virtual Host locate_file_metadata(const std::string& path) const = 0;
        virtual std::vector<Host> locate_directory_metadata(const std::string& path) const = 0;
};


class SimpleHashDistributor : public Distributor {
    private:
        Host localhost_;
        unsigned int hosts_size_;
        std::vector<Host> all_hosts_;
        std::hash<std::string> str_hash;
    public:
        SimpleHashDistributor(Host localhost, unsigned int hosts_size);
        Host localhost() const override;
        Host locate_data(const std::string& path, const ChunkID& chnk_id) const override;
        Host locate_file_metadata(const std::string& path) const override;
        std::vector<Host> locate_directory_metadata(const std::string& path) const override;
};

class LocalOnlyDistributor : public Distributor {
    private:
        Host localhost_;
    public:
        LocalOnlyDistributor(Host localhost);
        Host localhost() const override;
        Host locate_data(const std::string& path, const ChunkID& chnk_id) const override;
        Host locate_file_metadata(const std::string& path) const override;
        std::vector<Host> locate_directory_metadata(const std::string& path) const override;
};

#endif //IFS_RPC_LOCATOR_HPP
