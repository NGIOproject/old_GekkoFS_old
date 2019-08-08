/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef IFS_CHUNK_STORAGE_HPP
#define IFS_CHUNK_STORAGE_HPP

#include <abt.h>
#include <limits.h>
#include <string>
#include <memory>

/* Forward declarations */
namespace spdlog {
    class logger;
}


struct ChunkStat {
    unsigned long chunk_size;
    unsigned long chunk_total;
    unsigned long chunk_free;
};

class ChunkStorage {
    private:
        static constexpr const char * LOGGER_NAME = "ChunkStorage";

        std::shared_ptr<spdlog::logger> log;

        std::string root_path;
        size_t chunksize;
        inline std::string absolute(const std::string& internal_path) const;
        static inline std::string get_chunks_dir(const std::string& file_path);
        static inline std::string get_chunk_path(const std::string& file_path, unsigned int chunk_id);
        void init_chunk_space(const std::string& file_path) const;

    public:
        ChunkStorage(const std::string& path, const size_t chunksize);
        void write_chunk(const std::string& file_path, unsigned int chunk_id,
                         const char * buff, size_t size, off64_t offset,
                         ABT_eventual& eventual) const;
        void read_chunk(const std::string& file_path, unsigned int chunk_id,
                         char * buff, size_t size, off64_t offset,
                         ABT_eventual& eventual) const;
        void trim_chunk_space(const std::string& file_path, unsigned int chunk_start,
                unsigned int chunk_end = UINT_MAX);
        void delete_chunk(const std::string& file_path, unsigned int chunk_id);
        void truncate_chunk(const std::string& file_path, unsigned int chunk_id, off_t length);
        void destroy_chunk_space(const std::string& file_path) const;
        ChunkStat chunk_stat() const;
};

#endif //IFS_CHUNK_STORAGE_HPP
