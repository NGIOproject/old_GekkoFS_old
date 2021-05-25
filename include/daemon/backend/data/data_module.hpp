/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GEKKOFS_DAEMON_DATA_LOGGING_HPP
#define GEKKOFS_DAEMON_DATA_LOGGING_HPP

#include <spdlog/spdlog.h>

namespace gkfs {
namespace data {

class DataModule {

private:
    DataModule() {}

    std::shared_ptr<spdlog::logger> log_;

public:

    static constexpr const char* LOGGER_NAME = "DataModule";

    static DataModule* getInstance() {
        static DataModule instance;
        return &instance;
    }

    DataModule(DataModule const&) = delete;

    void operator=(DataModule const&) = delete;

    const std::shared_ptr<spdlog::logger>& log() const;

    void log(const std::shared_ptr<spdlog::logger>& log);

};

#define GKFS_DATA_MOD (static_cast<gkfs::data::DataModule*>(gkfs::data::DataModule::getInstance()))

} // namespace data
} // namespace gkfs

#endif //GEKKOFS_DAEMON_DATA_LOGGING_HPP
