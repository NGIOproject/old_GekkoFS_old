/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#ifndef IFS_PRELOAD_C_DATA_WS_HPP
#define IFS_PRELOAD_C_DATA_WS_HPP


namespace rpc_send {

ssize_t write(const std::string& path, const void* buf, const bool append_flag, const off64_t in_offset,
                       const size_t write_size, const int64_t updated_metadentry_size);
struct ChunkStat {
    unsigned long chunk_size;
    unsigned long chunk_total;
    unsigned long chunk_free;
};

ssize_t read(const std::string& path, void* buf, const off64_t offset, const size_t read_size);

int trunc_data(const std::string& path, size_t current_size, size_t new_size);

ChunkStat chunk_stat();

}


#endif //IFS_PRELOAD_C_DATA_WS_HPP
