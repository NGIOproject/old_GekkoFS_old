/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#ifndef LFS_RPC_DATA_HPP
#define LFS_RPC_DATA_HPP

#include <daemon/main.hpp>

class RPCData {

private:
    RPCData() {}

    // Margo IDs. They can also be used to retrieve the Mercury classes and contexts that were created at init time
    margo_instance_id server_rpc_mid_;

    // Argobots I/O pools and execution streams
    ABT_pool io_pool_;
    std::vector<ABT_xstream> io_streams_;
    std::string self_addr_str_;

public:

    static RPCData* getInstance() {
        static RPCData instance;
        return &instance;
    }

    RPCData(RPCData const&) = delete;

    void operator=(RPCData const&) = delete;

    // Getter/Setter

    margo_instance* server_rpc_mid();

    void server_rpc_mid(margo_instance* server_rpc_mid);

    ABT_pool io_pool() const;

    void io_pool(ABT_pool io_pool);

    std::vector<ABT_xstream>& io_streams();

    void io_streams(const std::vector<ABT_xstream>& io_streams);

    const std::string& self_addr_str() const;

    void self_addr_str(const std::string& addr_str);


};


#endif //LFS_RPC_DATA_HPP
